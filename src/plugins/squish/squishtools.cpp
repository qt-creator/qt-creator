// Copyright (C) 2022 The Qt Company Ltd
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "squishtools.h"

#include "scripthelper.h"
#include "squishoutputpane.h"
#include "squishplugin.h"
#include "squishsettings.h"
#include "squishtr.h"
#include "squishxmloutputhandler.h"

#include <QDebug> // TODO remove

#include <coreplugin/documentmanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>
#include <debugger/breakhandler.h>
#include <debugger/debuggerconstants.h>
#include <debugger/debuggericons.h>
#include <texteditor/textmark.h>

#include <utils/algorithm.h>
#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>
#include <utils/temporaryfile.h>
#include <utils/utilsicons.h>

#include <QApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileSystemWatcher>
#include <QLoggingCategory>
#include <QMessageBox>
#include <QTimer>
#include <QWindow>

static Q_LOGGING_CATEGORY(LOG, "qtc.squish.squishtools", QtWarningMsg)

using namespace Utils;

namespace Squish {
namespace Internal {

static QString runnerStateName(RunnerState state)
{
    switch (state) {
    case RunnerState::None: return "None";
    case RunnerState::Starting: return "Starting";
    case RunnerState::Running: return "Running";
    case RunnerState::RunRequested: return "RunRequested";
    case RunnerState::Interrupted: return "Interrupted";
    case RunnerState::InterruptRequested: return "InterruptedRequested";
    case RunnerState::Canceling: return "Canceling";
    case RunnerState::Canceled: return "Canceled";
    case RunnerState::CancelRequested: return "CancelRequested";
    case RunnerState::CancelRequestedWhileInterrupted: return "CancelRequestedWhileInterrupted";
    case RunnerState::Finished: return "Finished";
    }
    return "ThouShallNotBeHere";
}

static QString toolsStateName(SquishTools::State state)
{
    switch (state) {
    case SquishTools::Idle: return "Idle";
    case SquishTools::ServerStarting: return "ServerStarting";
    case SquishTools::ServerStarted: return "ServerStartFailed";
    case SquishTools::ServerStartFailed: return "ServerStopped";
    case SquishTools::ServerStopped: return "ServerStopped";
    case SquishTools::ServerStopFailed: return "ServerStopFailed";
    case SquishTools::RunnerStarting: return "RunnerStarting";
    case SquishTools::RunnerStarted: return "RunnerStarted";
    case SquishTools::RunnerStartFailed: return "RunnerStartFailed";
    case SquishTools::RunnerStopped: return "RunnerStopped";
    }
    return "UnexpectedState";
}

static void logRunnerStateChange(RunnerState from, RunnerState to)
{
    qCInfo(LOG) << "Runner state change:" << runnerStateName(from) << ">" << runnerStateName(to);
}

class SquishLocationMark : public TextEditor::TextMark
{
public:
    SquishLocationMark(const FilePath &filePath, int line)
        : TextEditor::TextMark(filePath, line, Id("Squish.LocationMark"))
    {
        setIsLocationMarker(true);
        setIcon(Debugger::Icons::LOCATION.icon());
        setPriority(HighPriority);
    }
};

// make this configurable?
static const Utils::FilePath resultsDirectory = Utils::FileUtils::homePath()
        .pathAppended(".squishQC/Test Results");
static SquishTools *s_instance = nullptr;

SquishTools::SquishTools(QObject *parent)
    : QObject(parent)
    , m_suiteConf(Utils::FilePath{})
{
    SquishOutputPane *outputPane = SquishOutputPane::instance();
    connect(this, &SquishTools::logOutputReceived,
            outputPane, &SquishOutputPane::addLogOutput, Qt::QueuedConnection);
    connect(this, &SquishTools::squishTestRunStarted,
            outputPane, &SquishOutputPane::clearOldResults);
    connect(this, &SquishTools::squishTestRunFinished,
            outputPane, &SquishOutputPane::onTestRunFinished);

    m_runnerProcess.setProcessMode(ProcessMode::Writer);
    m_recorderProcess.setProcessMode(ProcessMode::Writer);

    m_runnerProcess.setStdOutLineCallback([this](const QString &line) {
        onRunnerStdOutput(line);
    });

    connect(&m_recorderProcess, &QtcProcess::done,
            this, &SquishTools::onRecorderFinished);

    connect(&m_runnerProcess, &QtcProcess::readyReadStandardError,
            this, &SquishTools::onRunnerErrorOutput);
    connect(&m_runnerProcess, &QtcProcess::done,
            this, &SquishTools::onRunnerFinished);

    connect(&m_serverProcess, &SquishServerProcess::stateChanged,
            this, &SquishTools::onServerStateChanged);
    connect(&m_serverProcess, &SquishServerProcess::logOutputReceived,
            this, &SquishTools::logOutputReceived);

    s_instance = this;
    m_perspective.initPerspective();
    connect(&m_perspective, &SquishPerspective::interruptRequested,
            this, [this] {
        logRunnerStateChange(m_squishRunnerState, RunnerState::InterruptRequested);
        m_squishRunnerState = RunnerState::InterruptRequested;
        if (m_runnerProcess.processId() != -1)
            interruptRunner();
    });
    connect(&m_perspective, &SquishPerspective::stopRequested, this, [this] {
        bool interrupted = m_squishRunnerState == RunnerState::Interrupted;
        RunnerState state = interrupted ? RunnerState::CancelRequestedWhileInterrupted
                                        : RunnerState::CancelRequested;
        logRunnerStateChange(m_squishRunnerState, state);
        m_squishRunnerState = state;
        if (interrupted)
            handlePrompt();
        else if (m_runnerProcess.processId() != -1)
            terminateRunner();
    });
    connect(&m_perspective, &SquishPerspective::stopRecordRequested,
            this, &SquishTools::stopRecorder);
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

void SquishTools::runTestCases(const FilePath &suitePath,
                               const QStringList &testCases)
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
    if (!resultsDirectory.ensureWritableDir()) {
        QMessageBox::critical(Core::ICore::dialogParent(),
                              Tr::tr("Error"),
                              Tr::tr("Could not create test results folder. Canceling test run."));
        return;
    }

    m_suitePath = suitePath;
    m_suiteConf = SuiteConf::readSuiteConf(suitePath.pathAppended("suite.conf"));
    m_testCases = testCases;
    m_reportFiles.clear();

    const QString dateTimeString = QDateTime::currentDateTime().toString("yyyy-MM-ddTHH-mm-ss");
    m_currentResultsDirectory = resultsDirectory.pathAppended(dateTimeString);

    m_additionalRunnerArguments.clear();
    m_additionalRunnerArguments << "--interactive" << "--resultdir"
                                << m_currentResultsDirectory.toUserOutput();

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

void SquishTools::queryGlobalScripts(QueryCallback callback)
{
    m_queryCallback = callback;
    queryServer(GetGlobalScriptDirs);
}

void SquishTools::queryServerSettings(QueryCallback callback)
{
    m_queryCallback = callback;
    queryServer(ServerInfo);
}

void SquishTools::requestSetSharedFolders(const Utils::FilePaths &sharedFolders)
{
    // when sharedFolders is empty we need to pass an (explicit) empty string
    // otherwise a list of paths, for convenience we quote each path
    m_queryParameter = '"' + Utils::transform(sharedFolders, &FilePath::toUserOutput).join("\",\"") + '"';
    queryServer(SetGlobalScriptDirs);
}


void SquishTools::queryServer(RunnerQuery query)
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
    m_query = query;
    startSquishServer(RunnerQueryRequested);
}

void SquishTools::recordTestCase(const FilePath &suitePath, const QString &testCaseName,
                                 const SuiteConf &suiteConf)
{
    if (m_shutdownInitiated)
        return;
    if (m_state != Idle) {
        QMessageBox::critical(Core::ICore::dialogParent(),
                              Tr::tr("Error"),
                              Tr::tr("Squish Tools in unexpected state (%1).\n"
                                     "Refusing to record a test case.")
                                .arg(m_state));
        return;
    }

    m_suitePath = suitePath;
    m_testCases = {testCaseName};
    m_suiteConf = suiteConf;

    m_additionalRunnerArguments.clear();
    m_perspective.setPerspectiveMode(SquishPerspective::Recording);
    startSquishServer(RecordTestRequested);
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

void SquishTools::onServerStateChanged(SquishProcessState state)
{
    switch (state) {
    case Starting:
        setState(SquishTools::ServerStarting);
        break;
    case Started:
        setState(SquishTools::ServerStarted);
        break;
    case StartFailed:
        setState(SquishTools::ServerStartFailed);
        break;
    case Stopped:
        setState(SquishTools::ServerStopped);
        break;
    case StopFailed:
        setState(SquishTools::ServerStopFailed);
        break;
    default:
        // Idle currently unhandled / not needed?
        break;
    }
}

void SquishTools::setState(SquishTools::State state)
{
    qCInfo(LOG) << "State change:" << toolsStateName(m_state) << ">" << toolsStateName(state);
    // TODO check whether state transition is legal
    m_state = state;

    if (m_request == RecordTestRequested || m_request == KillOldBeforeRecordRunner) {
        handleSetStateStartAppRunner();
        return;
    } else if (m_request == RunnerQueryRequested || m_request == KillOldBeforeQueryRunner) {
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
        } else {
            QTC_ASSERT(false, qDebug() << m_state << m_request);
        }
        break;
    case ServerStopFailed:
        m_serverProcess.closeProcess();
        if (toolsSettings.minimizeIDE)
            restoreQtCreatorWindows();
        m_perspective.destroyControlBar();
        m_state = Idle;
        break;
    case RunnerStartFailed:
    case RunnerStopped:
        if (m_testCases.isEmpty() || (m_squishRunnerState == RunnerState::Canceled)) {
            m_request = ServerStopRequested;
            qCInfo(LOG) << "Stopping server from RunnerStopped";
            stopSquishServer();
            QString error;
            SquishXmlOutputHandler::mergeResultFiles(m_reportFiles,
                                                     m_currentResultsDirectory,
                                                     m_suitePath.fileName(),
                                                     &error);
            if (!error.isEmpty())
                QMessageBox::critical(Core::ICore::dialogParent(), Tr::tr("Error"), error);
            logrotateTestResults();
        } else {
            m_xmlOutputHandler->clearForNextRun();
            m_perspective.setPerspectiveMode(SquishPerspective::Running);
            logRunnerStateChange(m_squishRunnerState, RunnerState::Starting);
            m_squishRunnerState = RunnerState::Starting;
            startSquishRunner();
        }
        break;
    default:
        break;
    }
}

void SquishTools::handleSetStateStartAppRunner()
{
    switch (m_state) {
    case Idle:
        setIdle();
        break;
    case ServerStarted:
        startSquishRunner();
        break;
    case ServerStartFailed:
        m_state = Idle;
        m_request = None;
        if (toolsSettings.minimizeIDE)
            restoreQtCreatorWindows();
        m_perspective.destroyControlBar();
        break;
    case ServerStopped:
        m_state = Idle;
        emit shutdownFinished();
        if (m_request == ServerStopRequested) {
            m_request = None;
            if (toolsSettings.minimizeIDE)
                restoreQtCreatorWindows();
            m_perspective.destroyControlBar();
        } else if (m_request == KillOldBeforeRecordRunner) {
            startSquishServer(RecordTestRequested);
        } else {
            QTC_ASSERT(false, qDebug() << m_state << m_request);
        }
        break;
    case ServerStopFailed:
        m_serverProcess.closeProcess();
        if (toolsSettings.minimizeIDE)
            restoreQtCreatorWindows();
        m_perspective.destroyControlBar();
        m_state = Idle;
        break;
    case RunnerStartFailed:
    case RunnerStopped:
        if (m_recorderProcess.isRunning()) {
            stopRecorder();
        } else {
            m_request = ServerStopRequested;
            qCInfo(LOG) << "Stopping server from RunnerStopped (startaut)";
            stopSquishServer();
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
        qCInfo(LOG) << "Stopping server from RunnerStopped (query)";
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
    m_suitePath = {};
    m_testCases.clear();
    m_currentTestCasePath.clear();
    m_reportFiles.clear();
    m_additionalRunnerArguments.clear();
    m_perspective.setPerspectiveMode(SquishPerspective::NoMode);
    m_currentResultsDirectory.clear();
    m_lastTopLevelWindows.clear();
}

void SquishTools::startSquishServer(Request request)
{
    if (m_shutdownInitiated)
        return;
    m_licenseIssues = false;
    QTC_ASSERT(m_perspective.perspectiveMode() != SquishPerspective::NoMode, return);
    m_request = request;
    if (m_serverProcess.state() != QProcess::NotRunning) {
        handleSquishServerAlreadyRunning();
        return;
    }

    toolsSettings.setup();

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

    if (m_request == RunTestRequested || m_request == RecordTestRequested) {
        if (toolsSettings.minimizeIDE)
            minimizeQtCreatorWindows();
        else
            m_lastTopLevelWindows.clear();
        if (m_request == RunTestRequested && QTC_GUARD(m_xmlOutputHandler))
            m_perspective.showControlBar(m_xmlOutputHandler.get());
        else
            m_perspective.showControlBar(nullptr);

        m_perspective.select();
        logRunnerStateChange(m_squishRunnerState, RunnerState::Starting);
        m_squishRunnerState = RunnerState::Starting;
        if (m_request == RecordTestRequested)
            m_perspective.updateStatus(Tr::tr("Recording test case"));
    }

    const QStringList arguments = serverArgumentsFromSettings();
    m_serverProcess.start({toolsSettings.serverPath, arguments}, squishEnvironment());
}

void SquishTools::stopSquishServer()
{
    qCDebug(LOG) << "Stopping server";
    m_serverProcess.stop();
}

void SquishTools::startSquishRunner()
{
    if (!isValidToStartRunner() || !setupRunnerPath())
        return;

    const QStringList args = runnerArgumentsFromSettings();

    m_autId = 0;
    if (m_request == RecordTestRequested)
        m_closeRunnerOnEndRecord = true;

    Utils::CommandLine cmdLine = {toolsSettings.runnerPath, args};
    setupAndStartSquishRunnerProcess(cmdLine);
}

void SquishTools::setupAndStartRecorder()
{
    QTC_ASSERT(m_autId != 0, return);
    QTC_ASSERT(!m_recorderProcess.isRunning(), return);

    QStringList args;
    if (!toolsSettings.isLocalServer)
        args << "--host" << toolsSettings.serverHost;
    args << "--port" << QString::number(m_serverProcess.port());
    args << "--debugLog" << "alpw"; // TODO make this configurable?
    args << "--record";
    args << "--suitedir" << m_suitePath.toUserOutput();

    Utils::TemporaryFile tmp("squishsnippetfile-XXXXXX"); // quick and dirty
    tmp.open();
    m_currentRecorderSnippetFile = Utils::FilePath::fromUserInput(tmp.fileName());
    args << "--outfile" << m_currentRecorderSnippetFile.toUserOutput();
    tmp.close();
    args << "--lang" << m_suiteConf.langParameter();
    args << "--useWaitFor" << "--recordStart";
    if (m_suiteConf.objectMapStyle() == "script")
        args << "--useScriptedObjectMap";
    args << "--autid" << QString::number(m_autId);

    m_recorderProcess.setCommand({toolsSettings.runnerPath, args});
    qCDebug(LOG) << "Recorder starting:" << m_recorderProcess.commandLine().toUserOutput();
    if (m_suiteConf.objectMapPath().isReadableFile())
        Core::DocumentManager::expectFileChange(m_suiteConf.objectMapPath());
    m_recorderProcess.start();
}

void SquishTools::stopRecorder()
{
    QTC_ASSERT(m_recorderProcess.isRunning(), return);
    if (m_squishRunnerState == RunnerState::CancelRequested) {
        qCDebug(LOG) << "Stopping recorder (exit)";
        m_recorderProcess.write("exit\n");
    } else {
        qCDebug(LOG) << "Stopping recorder (endrecord)";
        m_recorderProcess.write("endrecord\n");
    }
}

void SquishTools::executeRunnerQuery()
{
    if (!isValidToStartRunner() || !setupRunnerPath())
        return;

    QStringList arguments = { "--port", QString::number(m_serverProcess.port()) };
    Utils::CommandLine cmdLine = {toolsSettings.runnerPath, arguments};
    switch (m_query) {
    case ServerInfo:
        cmdLine.addArg("--info");
        cmdLine.addArg("all");
        break;
    case GetGlobalScriptDirs:
        cmdLine.addArg("--config");
        cmdLine.addArg("getGlobalScriptDirs");
        break;
    case SetGlobalScriptDirs:
        cmdLine.addArg("--config");
        cmdLine.addArg("setGlobalScriptDirs");
        cmdLine.addArgs(m_queryParameter, Utils::CommandLine::Raw);
        break;
    default:
        QTC_ASSERT(false, return);
    }
    setupAndStartSquishRunnerProcess(cmdLine);
}

Environment SquishTools::squishEnvironment()
{
    Environment environment = Environment::systemEnvironment();
    if (!toolsSettings.licenseKeyPath.isEmpty())
        environment.prependOrSet("SQUISH_LICENSEKEY_DIR", toolsSettings.licenseKeyPath.nativePath());
    environment.prependOrSet("SQUISH_PREFIX", toolsSettings.squishPath.nativePath());
    return environment;
}

void SquishTools::onRunnerFinished()
{
    qCDebug(LOG) << "Runner finished";
    if (m_request == RunnerQueryRequested) {
        const QString error = m_licenseIssues ? Tr::tr("Could not get Squish license from server.")
                                              : QString();
        if (m_queryCallback)
            m_queryCallback(m_fullRunnerOutput, error);
        setState(RunnerStopped);
        m_fullRunnerOutput.clear();
        m_queryCallback = {};
        m_queryParameter.clear();
        return;
    }

    if (!m_shutdownInitiated) {
        logRunnerStateChange(m_squishRunnerState, RunnerState::Finished);
        m_squishRunnerState = RunnerState::Finished;
        if (m_request == RunTestRequested)
            m_perspective.updateStatus(Tr::tr("Test run finished."));
        else if (m_request == RecordTestRequested)
            m_perspective.updateStatus(Tr::tr("Test record finished."));
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

void SquishTools::onRecorderFinished()
{
    qCDebug(LOG) << "Recorder finished:" << m_recorderProcess.exitCode();
    if (m_runnerProcess.isRunning()) {
        if (m_closeRunnerOnEndRecord) {
            //terminateRunner();
            m_runnerProcess.write("exit\n"); // why doesn't work anymore?
        }
    } else {
        m_request = ServerStopRequested;
        qCInfo(LOG) << "Stop Server from recorder";
        stopSquishServer();
    }

    if (!m_currentRecorderSnippetFile.exists()) {
        qCInfo(LOG) << m_currentRecorderSnippetFile.toUserOutput() << "does not exist";
        return;
    }
    qCInfo(LOG).noquote() << "\nSnippetFile content:\n--------------------\n"
                          << m_currentRecorderSnippetFile.fileContents().value_or(QByteArray())
                          << "--------------------";

    const ScriptHelper helper(m_suiteConf.language());
    const Utils::FilePath testFile = m_currentTestCasePath.pathAppended(
                "test" + m_suiteConf.scriptExtension());
    Core::DocumentManager::expectFileChange(testFile);
    bool result = helper.writeScriptFile(testFile, m_currentRecorderSnippetFile,
                                         m_suiteConf.aut(),
                                         m_suiteConf.arguments());
    qCInfo(LOG) << "Wrote recorded test case" << testFile.toUserOutput() << " " << result;
    m_currentRecorderSnippetFile.removeFile();
    m_currentRecorderSnippetFile.clear();
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
    if (m_request != RunTestRequested) // only handle test runs here, query is handled on done
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
    const QByteArray output = m_runnerProcess.readAllRawStandardError();
    const QList<QByteArray> lines = output.split('\n');
    for (const QByteArray &line : lines) {
        const QByteArray trimmed = line.trimmed();
        if (!trimmed.isEmpty()) {
            emit logOutputReceived("Runner: " + QLatin1String(trimmed));
            if (trimmed.startsWith("QSocketNotifier: Invalid socket")) {
                // we've lost connection to the AUT - if Interrupted, try to cancel the runner
                if (m_squishRunnerState == RunnerState::Interrupted) {
                    logRunnerStateChange(m_squishRunnerState,
                                         RunnerState::CancelRequestedWhileInterrupted);
                    m_squishRunnerState = RunnerState::CancelRequestedWhileInterrupted;
                    handlePrompt();
                }
            } else if (trimmed.contains("could not be started.")
                       && trimmed.contains("Mapped AUT")) {
                QMessageBox::critical(Core::ICore::dialogParent(), Tr::tr("Error"),
                                      Tr::tr("Squish could not find the AUT \"%1\" to start. "
                                             "Make sure it has been added as a Mapped AUT in the "
                                             "squishserver settings.\n"
                                             "(Tools > Squish > Server Settings...)")
                                      .arg(m_suiteConf.aut()));
            } else if (trimmed.startsWith("Couldn't get license")
                       || trimmed.contains("UNLICENSED version of Squish")) {
                m_licenseIssues = true;
            }
        }
    }
}

void SquishTools::onRunnerStdOutput(const QString &lineIn)
{
    if (m_request == RunnerQueryRequested) { // only handle test runs / record here
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
    } else if (m_autId == 0 && line.startsWith("AUTID: ")) {
        isPrompt = true;
        m_autId = line.mid(7).toInt();
        qCInfo(LOG) << "AUT ID set" << m_autId << "(" << line << ")";
    }
    if (isPrompt)
        handlePrompt(fileName, fileLine, fileColumn);
}

// FIXME: add/removal of breakpoints while debugging not handled yet
// FIXME: enabled state of breakpoints
Utils::Links SquishTools::setBreakpoints()
{
    Utils::Links setBPs;
    using namespace Debugger::Internal;
    const GlobalBreakpoints globalBPs  = BreakpointManager::globalBreakpoints();
    const QString extension = m_suiteConf.scriptExtension();
    for (const GlobalBreakpoint &gb : globalBPs) {
        if (!gb->isEnabled())
            continue;
        const Utils::FilePath filePath = Utils::FilePath::fromString(
                    gb->data(BreakpointFileColumn, Qt::DisplayRole).toString());
        auto fileName = filePath.canonicalPath().toUserOutput();
        if (fileName.isEmpty())
            continue;
        if (!fileName.endsWith(extension))
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
        qCInfo(LOG).noquote().nospace() << "Setting breakpoint: '" << cmd << "'";
        m_runnerProcess.write(cmd);
        setBPs.append({filePath, line});
    }
    return setBPs;
}

void SquishTools::handlePrompt(const QString &fileName, int line, int column)
{
    if (m_perspective.perspectiveMode() == SquishPerspective::Recording) {
        switch (m_squishRunnerState) {
        case RunnerState::Starting:
            setupAndStartRecorder();
            onRunnerRunRequested(StepMode::Continue);
            break;
        case RunnerState::CancelRequested:
        case RunnerState::CancelRequestedWhileInterrupted:
            stopRecorder();
            logRunnerStateChange(m_squishRunnerState, RunnerState::Canceling);
            m_squishRunnerState = RunnerState::Canceling;
            break;
        case RunnerState::Canceled:
            QTC_CHECK(false);
            break;
        default:
            break;
        }
        return;
    }

    switch (m_squishRunnerState) {
    case RunnerState::Starting: {
        const Utils::Links setBPs = setBreakpoints();
        if (!setBPs.contains({Utils::FilePath::fromString(fileName), line})) {
            onRunnerRunRequested(StepMode::Continue);
        } else {
            m_perspective.setPerspectiveMode(SquishPerspective::Interrupted);
            logRunnerStateChange(m_squishRunnerState, RunnerState::Interrupted);
            m_squishRunnerState = RunnerState::Interrupted;
            restoreQtCreatorWindows();
            // request local variables
            m_runnerProcess.write("print variables\n");
            const FilePath filePath = FilePath::fromString(fileName);
            Core::EditorManager::openEditorAt({filePath, line, column});
            updateLocationMarker(filePath, line);
        }
        break;
    }
    case RunnerState::CancelRequested:
    case RunnerState::CancelRequestedWhileInterrupted:
        m_runnerProcess.write("exit\n");
        clearLocationMarker();
        logRunnerStateChange(m_squishRunnerState, RunnerState::Canceling);
        m_squishRunnerState = RunnerState::Canceling;
        break;
    case RunnerState::Canceling:
        m_runnerProcess.write("quit\n");
        logRunnerStateChange(m_squishRunnerState, RunnerState::Canceled);
        m_squishRunnerState = RunnerState::Canceled;
        break;
    case RunnerState::Canceled:
        QTC_CHECK(false);
        break;
    default:
        if (line != -1 && column != -1) {
            m_perspective.setPerspectiveMode(SquishPerspective::Interrupted);
            logRunnerStateChange(m_squishRunnerState, RunnerState::Interrupted);
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
                connect(m_requestVarsTimer, &QTimer::timeout, this, [this] {
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
        QTimer::singleShot(1000, this, [this, filePath] { onResultsDirChanged(filePath); });
    }
}

void SquishTools::logrotateTestResults()
{
    // make this configurable?
    const int maxNumberOfTestResults = 10;
    const Utils::FilePaths existing = resultsDirectory.dirEntries({{}, QDir::Dirs | QDir::NoDotAndDotDot},
                                                                  QDir::Name);

    for (int i = 0, limit = existing.size() - maxNumberOfTestResults; i < limit; ++i) {
        if (!existing.at(i).removeRecursively())
            qWarning() << "could not remove" << existing.at(i).toUserOutput();
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
                connect(window, &QWindow::destroyed, this, [this, window] {
                    m_lastTopLevelWindows.removeOne(window);
                });
            }
        }
    }
}

void SquishTools::restoreQtCreatorWindows()
{
    for (QWindow *window : std::as_const(m_lastTopLevelWindows)) {
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

void SquishTools::onRunnerRunRequested(StepMode step)
{
    if (m_requestVarsTimer) {
        delete m_requestVarsTimer;
        m_requestVarsTimer = nullptr;
    }
    logRunnerStateChange(m_squishRunnerState, RunnerState::RunRequested);
    m_squishRunnerState = RunnerState::RunRequested;

    if (step == StepMode::Continue)
        m_runnerProcess.write("continue\n");
    else if (step == StepMode::StepIn)
        m_runnerProcess.write("step\n");
    else if (step == StepMode::StepOver)
        m_runnerProcess.write("next\n");
    else if (step == StepMode::StepOut)
        m_runnerProcess.write("return\n");

    clearLocationMarker();
    if (toolsSettings.minimizeIDE)
        minimizeQtCreatorWindows();
    // avoid overriding Recording
    if (m_perspective.perspectiveMode() == SquishPerspective::Interrupted)
        m_perspective.setPerspectiveMode(SquishPerspective::Running);

    logRunnerStateChange(m_squishRunnerState, RunnerState::Running);
    m_squishRunnerState = RunnerState::Running;
}

void SquishTools::interruptRunner()
{
    qCDebug(LOG) << "Interrupting runner";
    const CommandLine cmd(toolsSettings.processComPath,
                          {QString::number(m_runnerProcess.processId()), "break"});
    QtcProcess process;
    process.setCommand(cmd);
    process.start();
    process.waitForFinished();
}

void SquishTools::terminateRunner()
{
    qCDebug(LOG) << "Terminating runner";
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

QStringList SquishTools::runnerArgumentsFromSettings()
{
    QStringList arguments;
    if (!toolsSettings.isLocalServer)
        arguments << "--host" << toolsSettings.serverHost;
    arguments << "--port" << QString::number(m_serverProcess.port());
    arguments << "--debugLog" << "alpw"; // TODO make this configurable?

    QTC_ASSERT(!m_testCases.isEmpty(), m_testCases.append(""));
    m_currentTestCasePath = m_suitePath / m_testCases.takeFirst();

    if (m_request == RecordTestRequested) {
        arguments << "--startapp"; // --record is triggered separately
    } else if (m_request == RunTestRequested) {
        arguments << "--testcase" << m_currentTestCasePath.toString();
        arguments << "--debug" << "--ide";
    } else {
        QTC_ASSERT(false, qDebug("Request %d", m_request));
    }

    arguments << "--suitedir" << m_suitePath.toUserOutput();

    arguments << m_additionalRunnerArguments;

    if (m_request == RecordTestRequested) {
        arguments << "--aut" << m_suiteConf.aut();
        if (!m_suiteConf.arguments().isEmpty())
            arguments << m_suiteConf.arguments().split(' ');
    }

    if (m_request == RunTestRequested) {
        const Utils::FilePath caseReportFilePath
                = m_currentResultsDirectory
                / m_suitePath.fileName() / m_currentTestCasePath.fileName() / "results.xml";
        m_reportFiles.append(caseReportFilePath);
        arguments << "--reportgen"
                  << QString::fromLatin1("xml2.2,%1").arg(caseReportFilePath.toUserOutput());

        m_currentResultsXML = new QFile(caseReportFilePath.toString());
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
    if (m_serverProcess.port() == -1) {
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

void SquishTools::setupAndStartSquishRunnerProcess(const Utils::CommandLine &cmdLine)
{
    m_runnerProcess.setCommand(cmdLine);
    m_runnerProcess.setEnvironment(squishEnvironment());
    setState(RunnerStarting);

    if (m_request == RunTestRequested) {
        // set up the file system watcher for being able to read the results.xml file
        m_resultsFileWatcher = new QFileSystemWatcher;
        // on 2nd run this directory exists and won't emit changes, so use the current subdirectory
        if (m_currentResultsDirectory.exists())
            m_resultsFileWatcher->addPath(m_currentResultsDirectory.pathAppended(m_suitePath.fileName()).toString());
        else
            m_resultsFileWatcher->addPath(m_currentResultsDirectory.toString());

        connect(m_resultsFileWatcher,
                &QFileSystemWatcher::directoryChanged,
                this,
                &SquishTools::onResultsDirChanged);
    }

    // especially when running multiple test cases we re-use the process fast and start the runner
    // several times and may crash as the process may not have been cleanly destructed yet
    m_runnerProcess.close();
    qCDebug(LOG) << "Runner starts:" << m_runnerProcess.commandLine().toUserOutput();
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
}

} // namespace Internal
} // namespace Squish
