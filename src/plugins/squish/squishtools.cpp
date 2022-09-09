// Copyright (C) 2022 The Qt Company Ltd
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "squishtools.h"

#include "squishoutputpane.h"
#include "squishplugin.h"
#include "squishsettings.h"
#include "squishtr.h"
#include "squishxmloutputhandler.h"

#include <QDebug> // TODO remove

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>
#include <debugger/breakhandler.h>
#include <debugger/debuggerconstants.h>
#include <debugger/debuggericons.h>
#include <texteditor/textmark.h>

#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>
#include <utils/utilsicons.h>

#include <QApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileSystemWatcher>
#include <QMessageBox>
#include <QTimer>
#include <QWindow>

using namespace Utils;

namespace Squish {
namespace Internal {


static QString runnerStateName(SquishTools::RunnerState state)
{
    switch (state) {
    case SquishTools::RunnerState::None: return "None";
    case SquishTools::RunnerState::Starting: return "Starting";
    case SquishTools::RunnerState::Running: return "Running";
    case SquishTools::RunnerState::RunRequested: return "RunRequested";
    case SquishTools::RunnerState::Interrupted: return "Interrupted";
    case SquishTools::RunnerState::InterruptRequested: return "InterruptedRequested";
    case SquishTools::RunnerState::Canceling: return "Canceling";
    case SquishTools::RunnerState::Canceled: return "Canceled";
    case SquishTools::RunnerState::CancelRequested: return "CancelRequested";
    case SquishTools::RunnerState::CancelRequestedWhileInterrupted: return "CancelRequestedWhileInterrupted";
    case SquishTools::RunnerState::Finished: return "Finished";
    }
    return "ThouShallNotBeHere";
}

class SquishLocationMark : public TextEditor::TextMark
{
public:
    SquishLocationMark(const FilePath &filePath, int line)
        : TextEditor::TextMark(filePath, line, Id("Squish.LocationMark"))
    {
        setIcon(Debugger::Icons::LOCATION.icon());
        setPriority(HighPriority);
    }
};

// make this configurable?
static const QString resultsDirectory = QFileInfo(QDir::home(), ".squishQC/Test Results")
                                            .absoluteFilePath();
static SquishTools *s_instance = nullptr;

SquishTools::SquishTools(QObject *parent)
    : QObject(parent)
{
    SquishOutputPane *outputPane = SquishOutputPane::instance();
    connect(this, &SquishTools::logOutputReceived,
            outputPane, &SquishOutputPane::addLogOutput, Qt::QueuedConnection);
    connect(this, &SquishTools::squishTestRunStarted,
            outputPane, &SquishOutputPane::clearOldResults);
    connect(this, &SquishTools::squishTestRunFinished,
            outputPane, &SquishOutputPane::onTestRunFinished);

    m_runnerProcess.setProcessMode(ProcessMode::Writer);

    m_runnerProcess.setStdOutLineCallback([this](const QString &line) {
        onRunnerStdOutput(line);
    });
    connect(&m_runnerProcess, &QtcProcess::readyReadStandardError,
            this, &SquishTools::onRunnerErrorOutput);
    connect(&m_runnerProcess, &QtcProcess::done,
            this, &SquishTools::onRunnerFinished);

    connect(&m_serverProcess, &QtcProcess::readyReadStandardOutput,
            this, &SquishTools::onServerOutput);
    connect(&m_serverProcess, &QtcProcess::readyReadStandardError,
            this, &SquishTools::onServerErrorOutput);
    connect(&m_serverProcess, &QtcProcess::done,
            this, &SquishTools::onServerFinished);
    s_instance = this;
    m_perspective.initPerspective();
    connect(&m_perspective, &SquishPerspective::interruptRequested,
            this, [this] {
        m_squishRunnerState = RunnerState::InterruptRequested;
        if (m_runnerProcess.processId() != -1)
            interruptRunner();
    });
    connect(&m_perspective, &SquishPerspective::stopRequested,
            this, [this] () {
        bool interrupted = m_squishRunnerState == RunnerState::Interrupted;
        m_squishRunnerState = interrupted ? RunnerState::CancelRequestedWhileInterrupted
                                          : RunnerState::CancelRequested;
        if (interrupted)
            handlePrompt();
        else if (m_runnerProcess.processId() != -1)
            terminateRunner();
    });
    connect(&m_perspective, &SquishPerspective::runRequested,
            this, &SquishTools::onRunnerRunRequested);
}

SquishTools::~SquishTools()
{
    if (m_locationMarker) // happens when QC is closed while Squish is executed
        delete m_locationMarker;
}

SquishTools *SquishTools::instance()
{
    QTC_CHECK(s_instance);
    return s_instance;
}

struct SquishToolsSettings
{
    SquishToolsSettings() {}

    FilePath squishPath;
    FilePath serverPath;
    FilePath runnerPath;
    FilePath processComPath;
    bool isLocalServer = true;
    bool verboseLog = false;
    bool minimizeIDE = true;
    QString serverHost = "localhost";
    int serverPort = 9999;
    FilePath licenseKeyPath;

    // populate members using current settings
    void setup()
    {
        const SquishSettings *squishSettings = SquishPlugin::squishSettings();
        QTC_ASSERT(squishSettings, return);
        squishPath = squishSettings->squishPath.filePath();

        if (!squishPath.isEmpty()) {
            const FilePath squishBin(squishPath.pathAppended("bin"));
            serverPath = squishBin.pathAppended(
                        HostOsInfo::withExecutableSuffix("squishserver")).absoluteFilePath();
            runnerPath = squishBin.pathAppended(
                        HostOsInfo::withExecutableSuffix("squishrunner")).absoluteFilePath();
            processComPath = squishBin.pathAppended(
                        HostOsInfo::withExecutableSuffix("processcomm")).absoluteFilePath();
        }

        isLocalServer = squishSettings->local.value();
        serverHost = squishSettings->serverHost.value();
        serverPort = squishSettings->serverPort.value();
        verboseLog = squishSettings->verbose.value();
        licenseKeyPath = squishSettings->licensePath.filePath();
        minimizeIDE = squishSettings->minimizeIDE.value();
    }
};

// make sure to execute setup() to populate with current settings before using it
static SquishToolsSettings toolsSettings;

void SquishTools::runTestCases(const QString &suitePath,
                               const QStringList &testCases,
                               const QStringList &additionalServerArgs,
                               const QStringList &additionalRunnerArgs)
{
    if (m_shutdownInitiated)
        return;
    if (m_state != Idle) {
        QMessageBox::critical(Core::ICore::dialogParent(),
                              Tr::tr("Error"),
                              Tr::tr("Squish Tools in unexpected state (%1).\n"
                                     "Refusing to run a test case.")
                                .arg(m_state));
        return;
    }
    // create test results directory (if necessary) and return on fail
    if (!QDir().mkpath(resultsDirectory)) {
        QMessageBox::critical(Core::ICore::dialogParent(),
                              Tr::tr("Error"),
                              Tr::tr("Could not create test results folder. Canceling test run."));
        return;
    }

    m_suitePath = suitePath;
    m_testCases = testCases;
    m_reportFiles.clear();
    m_additionalServerArguments = additionalServerArgs;

    const QString dateTimeString = QDateTime::currentDateTime().toString("yyyy-MM-ddTHH-mm-ss");
    m_currentResultsDirectory = QFileInfo(QDir(resultsDirectory), dateTimeString).absoluteFilePath();

    m_additionalRunnerArguments = additionalRunnerArgs;
    m_additionalRunnerArguments << "--interactive" << "--resultdir"
                                << QDir::toNativeSeparators(m_currentResultsDirectory);

    m_xmlOutputHandler.reset(new SquishXmlOutputHandler(this));
    connect(this, &SquishTools::resultOutputCreated,
            m_xmlOutputHandler.get(), &SquishXmlOutputHandler::outputAvailable,
            Qt::QueuedConnection);
    connect(m_xmlOutputHandler.get(), &SquishXmlOutputHandler::updateStatus,
            &m_perspective, &SquishPerspective::updateStatus);

    m_perspective.setPerspectiveMode(SquishPerspective::Running);
    emit squishTestRunStarted();
    startSquishServer(RunTestRequested);
}

void SquishTools::queryServerSettings()
{
    if (m_shutdownInitiated)
        return;
    if (m_state != Idle) {
        QMessageBox::critical(Core::ICore::dialogParent(),
                              Tr::tr("Error"),
                              Tr::tr("Squish Tools in unexpected state (%1).\n"
                                     "Refusing to execute server query.")
                                  .arg(m_state));
        return;
    }
    m_perspective.setPerspectiveMode(SquishPerspective::Querying);
    m_fullRunnerOutput.clear();
    startSquishServer(RunnerQueryRequested);
}

void SquishTools::writeServerSettingsChanges(const QList<QStringList> &changes)
{
    if (m_shutdownInitiated)
        return;
    if (m_state != Idle) {
        QMessageBox::critical(Core::ICore::dialogParent(),
                              Tr::tr("Error"),
                              Tr::tr("Squish Tools in unexpected state (%1).\n"
                                     "Refusing to write configuration changes.").arg(m_state));
        return;
    }
    m_serverConfigChanges = changes;
    startSquishServer(ServerConfigChangeRequested);
}

void SquishTools::setState(SquishTools::State state)
{
    // TODO check whether state transition is legal
    m_state = state;

    if (m_request == RunnerQueryRequested || m_request == KillOldBeforeQueryRunner) {
        handleSetStateQueryRunner();
        return;
    }

    switch (m_state) {
    case Idle:
        setIdle();
        break;
    case ServerStarted:
        if (m_request == RunTestRequested) {
            startSquishRunner();
        } else if (m_request == RecordTestRequested) {
        } else if (m_request == ServerConfigChangeRequested) { // nothing to do here
        } else {
            QTC_ASSERT(false, qDebug() << m_state << m_request);
        }
        break;
    case ServerStartFailed:
        m_state = Idle;
        if (m_request == RunTestRequested) {
            emit squishTestRunFinished();
            m_perspective.setPerspectiveMode(SquishPerspective::NoMode);
        }
        m_request = None;
        if (toolsSettings.minimizeIDE)
            restoreQtCreatorWindows();
        m_perspective.destroyControlBar();
        break;
    case ServerStopped:
        m_state = Idle;
        emit shutdownFinished();
        if (m_request == ServerConfigChangeRequested) {
            if (m_serverProcess.result() == ProcessResult::FinishedWithError) {
                emit configChangesFailed(m_serverProcess.error());
                break;
            }

            m_serverConfigChanges.removeFirst();
            if (!m_serverConfigChanges.isEmpty())
                startSquishServer(ServerConfigChangeRequested);
            else
                emit configChangesWritten();
        } else if (m_request == ServerStopRequested) {
            m_request = None;
            if (m_perspective.perspectiveMode() == SquishPerspective::Running) {
                emit squishTestRunFinished();
                m_perspective.setPerspectiveMode(SquishPerspective::NoMode);
            }
            if (toolsSettings.minimizeIDE)
                restoreQtCreatorWindows();
            m_perspective.destroyControlBar();
        } else if (m_request == KillOldBeforeRunRunner) {
            startSquishServer(RunTestRequested);
        } else if (m_request == KillOldBeforeRecordRunner) {
            startSquishServer(RecordTestRequested);
        } else {
            QTC_ASSERT(false, qDebug() << m_state << m_request);
        }
        break;
    case ServerStopFailed:
        m_serverProcess.close();
        if (toolsSettings.minimizeIDE)
            restoreQtCreatorWindows();
        m_perspective.destroyControlBar();
        m_state = Idle;
        break;
    case RunnerStartFailed:
    case RunnerStopped:
        if (m_testCases.isEmpty() || (m_squishRunnerState == RunnerState::Canceled)) {
            m_request = ServerStopRequested;
            stopSquishServer();
            QString error;
            SquishXmlOutputHandler::mergeResultFiles(m_reportFiles,
                                                     m_currentResultsDirectory,
                                                     QDir(m_suitePath).dirName(),
                                                     &error);
            if (!error.isEmpty())
                QMessageBox::critical(Core::ICore::dialogParent(), Tr::tr("Error"), error);
            logrotateTestResults();
        } else {
            m_xmlOutputHandler->clearForNextRun();
            m_squishRunnerState = RunnerState::Starting;
            startSquishRunner();
        }
        break;
    default:
        break;
    }
}

void SquishTools::handleSetStateQueryRunner()
{
    switch (m_state) {
    case Idle:
        setIdle();
        break;
    case ServerStarted:
        executeRunnerQuery();
        break;
    case ServerStartFailed:
        m_state = Idle;
        m_request = None;
        break;
    case ServerStopped:
        m_state = Idle;
        emit shutdownFinished();
        if (m_request == KillOldBeforeQueryRunner) {
            startSquishServer(RunnerQueryRequested);
        } else {
            QTC_ASSERT(false, qDebug() << m_state << m_request);
        }
        break;
    case ServerStopFailed:
        m_state = Idle;
        break;
    case RunnerStartFailed:
    case RunnerStopped:
        m_request = ServerStopRequested;
        stopSquishServer();
        break;
    default:
        break;
    }
}

void SquishTools::setIdle()
{
    QTC_ASSERT(m_state == Idle, return);
    m_request = None;
    m_suitePath = QString();
    m_testCases.clear();
    m_currentTestCasePath.clear();
    m_reportFiles.clear();
    m_additionalRunnerArguments.clear();
    m_additionalServerArguments.clear();
    m_perspective.setPerspectiveMode(SquishPerspective::NoMode);
    m_currentResultsDirectory.clear();
    m_lastTopLevelWindows.clear();
}

void SquishTools::startSquishServer(Request request)
{
    if (m_shutdownInitiated)
        return;
    QTC_ASSERT(m_perspective.perspectiveMode() != SquishPerspective::NoMode, return);
    m_request = request;
    if (m_serverProcess.state() != QProcess::NotRunning) {
        handleSquishServerAlreadyRunning();
        return;
    }

    toolsSettings.setup();
    m_serverPort = -1;

    const FilePath squishServer = Environment::systemEnvironment().searchInPath(
        toolsSettings.serverPath.toString());
    if (!squishServer.isExecutableFile()) {
        QMessageBox::critical(Core::ICore::dialogParent(),
                              Tr::tr("Squish Server Error"),
                              Tr::tr("\"%1\" could not be found or is not executable.\n"
                                     "Check the settings.")
                                  .arg(toolsSettings.serverPath.toUserOutput()));
        setState(Idle);
        return;
    }
    toolsSettings.serverPath = squishServer;

    if (m_request == RunTestRequested) {
        if (toolsSettings.minimizeIDE)
            minimizeQtCreatorWindows();
        else
            m_lastTopLevelWindows.clear();
        if (QTC_GUARD(m_xmlOutputHandler))
            m_perspective.showControlBar(m_xmlOutputHandler.get());

        m_perspective.select();
        m_squishRunnerState = RunnerState::Starting;
    }

    const QStringList arguments = serverArgumentsFromSettings();
    m_serverProcess.setCommand({toolsSettings.serverPath, arguments});

    m_serverProcess.setEnvironment(squishEnvironment());

    // especially when writing server config we re-use the process fast and start the server
    // several times and may crash as the process may not have been cleanly destructed yet
    m_serverProcess.close();
    setState(ServerStarting);
    m_serverProcess.start();
    if (!m_serverProcess.waitForStarted()) {
        setState(ServerStartFailed);
        qWarning() << "squishserver did not start within 30s";
    }
}

void SquishTools::stopSquishServer()
{
    if (m_serverProcess.state() != QProcess::NotRunning && m_serverPort > 0) {
        QtcProcess serverKiller;
        QStringList args;
        args << "--stop" << "--port" << QString::number(m_serverPort);
        serverKiller.setCommand({m_serverProcess.commandLine().executable(), args});
        serverKiller.setEnvironment(m_serverProcess.environment());
        serverKiller.start();
        if (!serverKiller.waitForFinished()) {
            qWarning() << "Could not shutdown server within 30s";
            setState(ServerStopFailed);
        }
    } else {
        qWarning() << "either no process running or port < 1?"
                   << m_serverProcess.state() << m_serverPort;
        setState(ServerStopFailed);
    }
}

void SquishTools::startSquishRunner()
{
    if (!isValidToStartRunner() || !setupRunnerPath())
        return;

    QStringList args;
    args << m_additionalServerArguments;
    if (!toolsSettings.isLocalServer)
        args << "--host" << toolsSettings.serverHost;
    args << "--port" << QString::number(m_serverPort);
    args << "--debugLog" << "alpw"; // TODO make this configurable?

    m_currentTestCasePath = FilePath::fromString(m_suitePath) / m_testCases.takeFirst();
    args << "--testcase" << m_currentTestCasePath.toString();
    args << "--suitedir" << m_suitePath;
    args << "--debug" << "--ide";

    args << m_additionalRunnerArguments;

    const QString caseReportFilePath = QFileInfo(QString::fromLatin1("%1/%2/%3/results.xml")
                                                     .arg(m_currentResultsDirectory,
                                                          QDir(m_suitePath).dirName(),
                                                          m_currentTestCasePath.baseName()))
                                           .absoluteFilePath();
    m_reportFiles.append(caseReportFilePath);

    args << "--reportgen"
         << QString::fromLatin1("xml2.2,%1").arg(caseReportFilePath);

    setupAndStartSquishRunnerProcess(args, caseReportFilePath);
}

void SquishTools::executeRunnerQuery()
{
    if (!isValidToStartRunner() || !setupRunnerPath())
        return;

    setupAndStartSquishRunnerProcess({ "--port", QString::number(m_serverPort), "--info", "all"});
}

Environment SquishTools::squishEnvironment()
{
    Environment environment = Environment::systemEnvironment();
    if (!toolsSettings.licenseKeyPath.isEmpty())
        environment.prependOrSet("SQUISH_LICENSEKEY_DIR", toolsSettings.licenseKeyPath.nativePath());
    environment.prependOrSet("SQUISH_PREFIX", toolsSettings.squishPath.nativePath());
    return environment;
}

void SquishTools::onServerFinished()
{
    m_serverPort = -1;
    setState(ServerStopped);
}

void SquishTools::onRunnerFinished()
{
    if (m_request == RunnerQueryRequested) {
        emit queryFinished(m_fullRunnerOutput);
        setState(RunnerStopped);
        m_fullRunnerOutput.clear();
        return;
    }

    if (!m_shutdownInitiated) {
        m_squishRunnerState = RunnerState::Finished;
        m_perspective.updateStatus(Tr::tr("Test run finished."));
        m_perspective.setPerspectiveMode(SquishPerspective::NoMode);
    }

    if (m_resultsFileWatcher) {
        delete m_resultsFileWatcher;
        m_resultsFileWatcher = nullptr;
    }
    if (m_currentResultsXML) {
        // make sure results are being read if not done yet
        if (m_currentResultsXML->exists() && !m_currentResultsXML->isOpen())
            onResultsDirChanged(m_currentResultsXML->fileName());
        if (m_currentResultsXML->isOpen())
            m_currentResultsXML->close();
        delete m_currentResultsXML;
        m_currentResultsXML = nullptr;
    }
    setState(RunnerStopped);
}

void SquishTools::onServerOutput()
{
    // output used for getting the port information of the current squishserver
    const QByteArray output = m_serverProcess.readAllStandardOutput();
    const QList<QByteArray> lines = output.split('\n');
    for (const QByteArray &line : lines) {
        const QByteArray trimmed = line.trimmed();
        if (trimmed.isEmpty())
            continue;
        if (trimmed.startsWith("Port:")) {
            if (m_serverPort == -1) {
                bool ok;
                int port = trimmed.mid(6).toInt(&ok);
                if (ok) {
                    m_serverPort = port;
                    setState(ServerStarted);
                } else {
                    qWarning() << "could not get port number" << trimmed.mid(6);
                    setState(ServerStartFailed);
                }
            } else {
                qWarning() << "got a Port output - don't know why...";
            }
        }
        emit logOutputReceived(QString("Server: ") + QLatin1String(trimmed));
    }
}

void SquishTools::onServerErrorOutput()
{
    // output that must be send to the Runner/Server Log
    const QByteArray output = m_serverProcess.readAllStandardError();
    const QList<QByteArray> lines = output.split('\n');
    for (const QByteArray &line : lines) {
        const QByteArray trimmed = line.trimmed();
        if (!trimmed.isEmpty())
            emit logOutputReceived(QString("Server: ") + QLatin1String(trimmed));
    }
}

static char firstNonWhitespace(const QByteArray &text)
{
    for (int i = 0, limit = text.size(); i < limit; ++i)
        if (isspace(text.at(i)))
            continue;
        else
            return text.at(i);
    return 0;
}

static int positionAfterLastClosingTag(const QByteArray &text)
{
    QList<QByteArray> possibleEndTags;
    possibleEndTags << "</description>"
                    << "</message>"
                    << "</verification>"
                    << "</result>"
                    << "</test>"
                    << "</prolog>"
                    << "</epilog>"
                    << "</SquishReport>";

    int positionStart = text.lastIndexOf("</");
    if (positionStart == -1)
        return -1;

    int positionEnd = text.indexOf('>', positionStart);
    if (positionEnd == -1)
        return -1;

    QByteArray endTag = text.mid(positionStart, positionEnd + 1 - positionStart);
    if (possibleEndTags.contains(endTag))
        return positionEnd + 1;

    return positionAfterLastClosingTag(text.mid(0, positionStart));
}

void SquishTools::onRunnerOutput()
{
    if (m_request == RunnerQueryRequested) // only handle test runs here, query is handled on done
        return;

    // buffer for already read, but not processed content
    static QByteArray buffer;
    const qint64 currentSize = m_currentResultsXML->size();

    if (currentSize <= m_readResultsCount)
        return;

    QByteArray output = m_currentResultsXML->read(currentSize - m_readResultsCount);
    if (output.isEmpty())
        return;

    if (!buffer.isEmpty())
        output.prepend(buffer);
    // we might read only partial written stuff - so we have to figure out how much we can
    // pass on for further processing and buffer the rest for the next reading
    const int endTag = positionAfterLastClosingTag(output);
    if (endTag < output.size()) {
        buffer = output.mid(endTag);
        output.truncate(endTag);
    } else {
        buffer.clear();
    }

    m_readResultsCount += output.size();

    if (firstNonWhitespace(output) == '<') {
        // output that must be used for the TestResultsPane
        emit resultOutputCreated(output);
    } else {
        const QList<QByteArray> lines = output.split('\n');
        for (const QByteArray &line : lines) {
            const QByteArray trimmed = line.trimmed();
            if (!trimmed.isEmpty())
                emit logOutputReceived("Runner: " + QLatin1String(trimmed));
        }
    }
}

void SquishTools::onRunnerErrorOutput()
{
    // output that must be send to the Runner/Server Log
    const QByteArray output = m_runnerProcess.readAllStandardError();
    const QList<QByteArray> lines = output.split('\n');
    for (const QByteArray &line : lines) {
        const QByteArray trimmed = line.trimmed();
        if (!trimmed.isEmpty()) {
            emit logOutputReceived("Runner: " + QLatin1String(trimmed));
            if (trimmed.startsWith("QSocketNotifier: Invalid socket")) {
                // we've lost connection to the AUT - if Interrupted, try to cancel the runner
                if (m_squishRunnerState == RunnerState::Interrupted) {
                    m_squishRunnerState = RunnerState::CancelRequestedWhileInterrupted;
                    handlePrompt();
                }
            }
        }
    }
}

void SquishTools::onRunnerStdOutput(const QString &lineIn)
{
    if (m_request == RunnerQueryRequested) { // only handle test runs here
        m_fullRunnerOutput.append(lineIn);   // but store output for query, see onRunnerFinished()
        return;
    }

    int fileLine = -1;
    int fileColumn = -1;
    QString fileName;
    // we might enter this function by invoking it directly instead of getting signaled
    bool isPrompt = false;
    QString line = lineIn;
    line.chop(1); // line has a newline
    if (line.startsWith("SDBG:"))
        line = line.mid(5);
    if (line.isEmpty()) // we have a prompt
        isPrompt = true;
    else if (line.startsWith("symb")) { // symbols information (locals)
        isPrompt = true;
        // paranoia
        if (!line.endsWith("}"))
            return;
        if (line.at(4) == '.') { // single symbol information
            line = line.mid(5);
            emit symbolUpdated(line);
        } else { // lline.at(4) == ':' // all locals
            line = line.mid(6);
            line.chop(1);
            emit localsUpdated(line);
        }
    } else if (line.startsWith("@line")) { // location information (interrupted)
        isPrompt = true;
        // paranoia
        if (!line.endsWith(":"))
            return;

        const QStringList locationParts = line.split(',');
        QTC_ASSERT(locationParts.size() == 3, return);
        fileLine = locationParts[0].mid(6).toInt();
        fileColumn = locationParts[1].mid(7).toInt();
        fileName = locationParts[2].trimmed();
        fileName.chop(1); // remove the colon
        const FilePath fp = FilePath::fromString(fileName);
        if (fp.isRelativePath())
            fileName = m_currentTestCasePath.resolvePath(fileName).toString();
    }
    if (isPrompt)
        handlePrompt(fileName, fileLine, fileColumn);
}

// FIXME: add/removal of breakpoints while debugging not handled yet
// FIXME: enabled state of breakpoints
void SquishTools::setBreakpoints()
{
    using namespace Debugger::Internal;
    const GlobalBreakpoints globalBPs  = BreakpointManager::globalBreakpoints();
    for (const GlobalBreakpoint &gb : globalBPs) {
        if (!gb->isEnabled())
            continue;
        auto fileName = Utils::FilePath::fromUserInput(
                    gb->data(BreakpointFileColumn, Qt::DisplayRole).toString()).toUserOutput();
        if (fileName.isEmpty())
            continue;
        // mask backslashes and spaces
        fileName.replace('\\', "\\\\");
        fileName.replace(' ', "\\x20");
        auto line = gb->data(BreakpointLineColumn, Qt::DisplayRole).toInt();
        QString cmd = "break ";
        cmd.append(fileName);
        cmd.append(':');
        cmd.append(QString::number(line));
        cmd.append('\n');
        m_runnerProcess.write(cmd);
    }
}

void SquishTools::handlePrompt(const QString &fileName, int line, int column)
{
    switch (m_squishRunnerState) {
    case RunnerState::Starting:
        setBreakpoints();
        onRunnerRunRequested(SquishPerspective::Continue);
        break;
    case RunnerState::CancelRequested:
    case RunnerState::CancelRequestedWhileInterrupted:
        m_runnerProcess.write("exit\n");
        clearLocationMarker();
        m_squishRunnerState = RunnerState::Canceling;
        break;
    case RunnerState::Canceling:
        m_runnerProcess.write("quit\n");
        m_squishRunnerState = RunnerState::Canceled;
        break;
    case RunnerState::Canceled:
        QTC_CHECK(false);
        break;
    default:
        if (line != -1 && column != -1) {
            m_perspective.setPerspectiveMode(SquishPerspective::Interrupted);
            m_squishRunnerState = RunnerState::Interrupted;
            restoreQtCreatorWindows();
            // if we're returning from a function we might end up without a file information
            if (fileName.isEmpty()) {
                m_runnerProcess.write("next\n");
            } else {
                // request local variables
                m_runnerProcess.write("print variables\n");
                const FilePath filePath = FilePath::fromString(fileName);
                Core::EditorManager::openEditorAt({filePath, line, column});
                updateLocationMarker(filePath, line);
            }
        } else { // it's just some output coming from the server
            if (m_squishRunnerState == RunnerState::Interrupted && !m_requestVarsTimer) {
                // FIXME: this should be easier, but when interrupted and AUT is closed
                // runner does not get notified until continued/canceled
                m_requestVarsTimer = new QTimer(this);
                m_requestVarsTimer->setSingleShot(true);
                m_requestVarsTimer->setInterval(1000);
                connect(m_requestVarsTimer, &QTimer::timeout, this, [this]() {
                    m_runnerProcess.write("print variables\n");
                });
                m_requestVarsTimer->start();
            }
        }
    }
}

void SquishTools::requestExpansion(const QString &name)
{
    QTC_ASSERT(m_squishRunnerState == RunnerState::Interrupted, return);
    m_runnerProcess.write("print variables +" + name + "\n");
}

bool SquishTools::shutdown()
{
    QTC_ASSERT(!m_shutdownInitiated, return true);
    m_shutdownInitiated = true;
    if (m_runnerProcess.isRunning())
        terminateRunner();
    if (m_serverProcess.isRunning())
        m_serverProcess.stop();
    return !(m_serverProcess.isRunning() || m_runnerProcess.isRunning());
}

void SquishTools::onResultsDirChanged(const QString &filePath)
{
    if (!m_currentResultsXML)
        return; // runner finished before, m_currentResultsXML deleted

    if (m_currentResultsXML->exists()) {
        delete m_resultsFileWatcher;
        m_resultsFileWatcher = nullptr;
        m_readResultsCount = 0;
        if (m_currentResultsXML->open(QFile::ReadOnly)) {
            m_resultsFileWatcher = new QFileSystemWatcher;
            m_resultsFileWatcher->addPath(m_currentResultsXML->fileName());
            connect(m_resultsFileWatcher,
                    &QFileSystemWatcher::fileChanged,
                    this,
                    &SquishTools::onRunnerOutput);
            // squishrunner might have finished already, call once at least
            onRunnerOutput();
        } else {
            // TODO set a flag to process results.xml as soon the complete test run has finished
            qWarning() << "could not open results.xml although it exists" << filePath
                       << m_currentResultsXML->error() << m_currentResultsXML->errorString();
        }
    } else {
        disconnect(m_resultsFileWatcher);
        // results.xml is created as soon some output has been opened - so try again in a second
        QTimer::singleShot(1000, this, [this, filePath]() { onResultsDirChanged(filePath); });
    }
}

void SquishTools::logrotateTestResults()
{
    // make this configurable?
    const int maxNumberOfTestResults = 10;
    const QStringList existing = QDir(resultsDirectory)
                                     .entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);

    for (int i = 0, limit = existing.size() - maxNumberOfTestResults; i < limit; ++i) {
        QDir current(resultsDirectory + QDir::separator() + existing.at(i));
        if (!current.removeRecursively())
            qWarning() << "could not remove" << current.absolutePath();
    }
}

void SquishTools::minimizeQtCreatorWindows()
{
    const QWindowList topLevelWindows = QApplication::topLevelWindows();
    for (QWindow *window : topLevelWindows) {
        if (window->flags() & Qt::WindowStaysOnTopHint)
            continue;
        if (window->isVisible()) {
            window->showMinimized();
            if (!m_lastTopLevelWindows.contains(window)) {
                m_lastTopLevelWindows.append(window);
                connect(window, &QWindow::destroyed, this, [this, window]() {
                    m_lastTopLevelWindows.removeOne(window);
                });
            }
        }
    }
}

void SquishTools::restoreQtCreatorWindows()
{
    for (QWindow *window : qAsConst(m_lastTopLevelWindows)) {
        window->raise();
        window->requestActivate();
        window->showNormal();
    }
}

void SquishTools::updateLocationMarker(const Utils::FilePath &file, int line)
{
    if (QTC_GUARD(!m_locationMarker)) {
        m_locationMarker = new SquishLocationMark(file, line);
    } else {
        m_locationMarker->updateFileName(file);
        m_locationMarker->move(line);
    }
}

void SquishTools::clearLocationMarker()
{
    delete m_locationMarker;
    m_locationMarker = nullptr;
}

void SquishTools::onRunnerRunRequested(SquishPerspective::StepMode step)
{
    if (m_requestVarsTimer) {
        delete m_requestVarsTimer;
        m_requestVarsTimer = nullptr;
    }
    m_squishRunnerState = RunnerState::RunRequested;

    if (step == SquishPerspective::Continue)
        m_runnerProcess.write("continue\n");
    else if (step == SquishPerspective::StepIn)
        m_runnerProcess.write("step\n");
    else if (step == SquishPerspective::StepOver)
        m_runnerProcess.write("next\n");
    else if (step == SquishPerspective::StepOut)
        m_runnerProcess.write("return\n");

    clearLocationMarker();
    if (toolsSettings.minimizeIDE)
        minimizeQtCreatorWindows();
    m_perspective.setPerspectiveMode(SquishPerspective::Running);
    m_squishRunnerState = RunnerState::Running;
}

void SquishTools::interruptRunner()
{
    const CommandLine cmd(toolsSettings.processComPath,
                          {QString::number(m_runnerProcess.processId()), "break"});
    QtcProcess process;
    process.setCommand(cmd);
    process.start();
    process.waitForFinished();
}

void SquishTools::terminateRunner()
{
    m_testCases.clear();
    m_currentTestCasePath.clear();
    m_perspective.updateStatus(Tr::tr("User stop initiated."));
    // should we terminate the AUT instead of the runner?!?
    const CommandLine cmd(toolsSettings.processComPath,
                          {QString::number(m_runnerProcess.processId()), "terminate"});
    QtcProcess process;
    process.setCommand(cmd);
    process.start();
    process.waitForFinished();
}

void SquishTools::handleSquishServerAlreadyRunning()
{
    if (QMessageBox::question(Core::ICore::dialogParent(),
                              Tr::tr("Squish Server Already Running"),
                              Tr::tr("There is still an old Squish server instance running.\n"
                                     "This will cause problems later on.\n\n"
                                     "If you continue, the old instance will be terminated.\n"
                                     "Do you want to continue?"))
        == QMessageBox::Yes) {
        switch (m_request) {
        case RunTestRequested:
            m_request = KillOldBeforeRunRunner;
            break;
        case RecordTestRequested:
            m_request = KillOldBeforeRecordRunner;
            break;
        case RunnerQueryRequested:
            m_request = KillOldBeforeQueryRunner;
            break;
        default:
            QMessageBox::critical(Core::ICore::dialogParent(),
                                  Tr::tr("Error"),
                                  Tr::tr("Unexpected state or request while starting Squish "
                                         "server. (state: %1, request: %2)")
                                      .arg(m_state)
                                      .arg(m_request));
        }
        stopSquishServer();
    }
}

QStringList SquishTools::serverArgumentsFromSettings() const
{
    QStringList arguments;
    // TODO if isLocalServer is false we should start a squishserver on remote device
    if (toolsSettings.isLocalServer) {
        if (m_request != ServerConfigChangeRequested)
            arguments << "--local"; // for now - although Squish Docs say "don't use it"
    } else {
        arguments << "--port" << QString::number(toolsSettings.serverPort);
    }
    if (toolsSettings.verboseLog)
        arguments << "--verbose";

    if (m_request == ServerConfigChangeRequested && QTC_GUARD(!m_serverConfigChanges.isEmpty())) {
        arguments.append("--config");
        arguments.append(m_serverConfigChanges.first());
    }
    return arguments;
}

bool SquishTools::isValidToStartRunner()
{
    if (!m_serverProcess.isRunning()) {
        QMessageBox::critical(Core::ICore::dialogParent(),
                              Tr::tr("No Squish Server"),
                              Tr::tr("Squish server does not seem to be running.\n"
                                     "(state: %1, request: %2)\n"
                                     "Try again.")
                                  .arg(m_state)
                                  .arg(m_request));
        setState(Idle);
        return false;
    }
    if (m_serverPort == -1) {
        QMessageBox::critical(Core::ICore::dialogParent(),
                              Tr::tr("No Squish Server Port"),
                              Tr::tr("Failed to get the server port.\n"
                                     "(state: %1, request: %2)\n"
                                     "Try again.")
                                  .arg(m_state)
                                  .arg(m_request));
        // setting state to ServerStartFailed will terminate/kill the current unusable server
        setState(ServerStartFailed);
        return false;
    }

    if (m_runnerProcess.state() != QProcess::NotRunning) {
        QMessageBox::critical(Core::ICore::dialogParent(),
                              Tr::tr("Squish Runner Running"),
                              Tr::tr("Squish runner seems to be running already.\n"
                                     "(state: %1, request: %2)\n"
                                     "Wait until it has finished and try again.")
                                  .arg(m_state)
                                  .arg(m_request));
        return false;
    }
    return true;
}

bool SquishTools::setupRunnerPath()
{
    const FilePath squishRunner = Environment::systemEnvironment().searchInPath(
        toolsSettings.runnerPath.toString());
    if (!squishRunner.isExecutableFile()) {
        QMessageBox::critical(Core::ICore::dialogParent(),
                              Tr::tr("Squish Runner Error"),
                              Tr::tr("\"%1\" could not be found or is not executable.\n"
                                     "Check the settings.")
                                  .arg(toolsSettings.runnerPath.toUserOutput()));
        setState(RunnerStopped);
        return false;
    }
    toolsSettings.runnerPath = squishRunner;
    return true;
}

void SquishTools::setupAndStartSquishRunnerProcess(const QStringList &args,
                                                   const QString &caseReportFilePath)
{
    m_runnerProcess.setCommand({toolsSettings.runnerPath, args});
    m_runnerProcess.setEnvironment(squishEnvironment());

    setState(RunnerStarting);

    if (m_request == RunTestRequested) {
        // set up the file system watcher for being able to read the results.xml file
        m_resultsFileWatcher = new QFileSystemWatcher;
        // on 2nd run this directory exists and won't emit changes, so use the current subdirectory
        if (QDir(m_currentResultsDirectory).exists())
            m_resultsFileWatcher->addPath(m_currentResultsDirectory + QDir::separator()
                                          + QDir(m_suitePath).dirName());
        else
            m_resultsFileWatcher->addPath(QFileInfo(m_currentResultsDirectory).absolutePath());

        connect(m_resultsFileWatcher,
                &QFileSystemWatcher::directoryChanged,
                this,
                &SquishTools::onResultsDirChanged);
    }

    m_runnerProcess.start();
    if (!m_runnerProcess.waitForStarted()) {
        QMessageBox::critical(Core::ICore::dialogParent(),
                              Tr::tr("Squish Runner Error"),
                              Tr::tr("Squish runner failed to start within given timeframe."));
        delete m_resultsFileWatcher;
        m_resultsFileWatcher = nullptr;
        setState(RunnerStartFailed);
        m_runnerProcess.close();
        return;
    }
    setState(RunnerStarted);
    if (m_request == RunTestRequested)
        m_currentResultsXML = new QFile(caseReportFilePath);
}

} // namespace Internal
} // namespace Squish
