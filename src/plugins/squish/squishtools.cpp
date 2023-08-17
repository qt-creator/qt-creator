// Copyright (C) 2022 The Qt Company Ltd
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "squishtools.h"

#include "scripthelper.h"
#include "squishmessages.h"
#include "squishoutputpane.h"
#include "squishsettings.h"
#include "squishtr.h"
#include "squishxmloutputhandler.h"

#include <QDebug> // TODO remove

#include <coreplugin/documentmanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>
#include <debugger/debuggerconstants.h>
#include <debugger/debuggericons.h>
#include <texteditor/textmark.h>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>
#include <utils/temporaryfile.h>
#include <utils/utilsicons.h>

#include <QApplication>
#include <QDateTime>
#include <QFile>
#include <QFileSystemWatcher>
#include <QLoggingCategory>
#include <QTimer>
#include <QWindow>

static Q_LOGGING_CATEGORY(LOG, "qtc.squish.squishtools", QtWarningMsg)

using namespace Utils;

namespace Squish::Internal {

static QString runnerStateName(RunnerState state)
{
    switch (state) {
    case RunnerState::None: return "None";
    case RunnerState::Starting: return "Starting";
    case RunnerState::Running: return "Running";
    case RunnerState::RunRequested: return "RunRequested";
    case RunnerState::Interrupted: return "Interrupted";
    case RunnerState::InterruptRequested: return "InterruptedRequested";
    case RunnerState::CancelRequested: return "CancelRequested";
    case RunnerState::CancelRequestedWhileInterrupted: return "CancelRequestedWhileInterrupted";
    case RunnerState::Canceled: return "Canceled";
    case RunnerState::Finished: return "Finished";
    }
    return "ThouShallNotBeHere";
}

static QString toolsStateName(SquishTools::State state)
{
    switch (state) {
    case SquishTools::Idle: return "Idle";
    case SquishTools::ServerStarting: return "ServerStarting";
    case SquishTools::ServerStarted: return "ServerStarted";
    case SquishTools::ServerStartFailed: return "ServerStartFailed";
    case SquishTools::ServerStopped: return "ServerStopped";
    case SquishTools::ServerStopFailed: return "ServerStopFailed";
    case SquishTools::RunnerStarting: return "RunnerStarting";
    case SquishTools::RunnerStarted: return "RunnerStarted";
    case SquishTools::RunnerStartFailed: return "RunnerStartFailed";
    case SquishTools::RunnerStopped: return "RunnerStopped";
    }
    return "UnexpectedState";
}

class SquishLocationMark : public TextEditor::TextMark
{
public:
    SquishLocationMark(const FilePath &filePath, int line)
        : TextEditor::TextMark(filePath, line, {Tr::tr("Squish"), Id("Squish.LocationMark")})
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

    connect(&m_serverProcess, &SquishServerProcess::stateChanged,
            this, &SquishTools::onServerStateChanged);
    connect(&m_serverProcess, &SquishServerProcess::logOutputReceived,
            this, &SquishTools::logOutputReceived);
    connect(&m_serverProcess, &SquishServerProcess::portRetrieved,
            this, &SquishTools::onServerPortRetrieved);

    s_instance = this;
    m_perspective.initPerspective();
    connect(&m_perspective, &SquishPerspective::interruptRequested,
            this, [this] {
        logAndChangeRunnerState(RunnerState::InterruptRequested);
        if (m_primaryRunner && m_primaryRunner->processId() != -1)
            interruptRunner();
    });
    connect(&m_perspective, &SquishPerspective::stopRequested, this, [this] {
        bool interrupted = m_squishRunnerState == RunnerState::Interrupted;
        RunnerState state = interrupted ? RunnerState::CancelRequestedWhileInterrupted
                                        : RunnerState::CancelRequested;
        logAndChangeRunnerState(state);
        if (interrupted)
            handlePrompt();
        else if (m_primaryRunner && m_primaryRunner->processId() != -1)
            terminateRunner();
    });
    connect(&m_perspective, &SquishPerspective::stopRecordRequested,
            this, &SquishTools::stopRecorder);
    connect(&m_perspective, &SquishPerspective::runRequested,
            this, &SquishTools::onRunnerRunRequested);
    connect(&m_perspective, &SquishPerspective::inspectTriggered,
            this, &SquishTools::onInspectTriggered);
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
        squishPath = settings().squishPath();

        if (!squishPath.isEmpty()) {
            const FilePath squishBin = squishPath.pathAppended("bin").absoluteFilePath();
            serverPath = squishBin.pathAppended("squishserver").withExecutableSuffix();
            runnerPath = squishBin.pathAppended("squishrunner").withExecutableSuffix();
            processComPath = squishBin.pathAppended("processcomm").withExecutableSuffix();
        }

        isLocalServer = settings().local();
        serverHost = settings().serverHost();
        serverPort = settings().serverPort();
        verboseLog = settings().verbose();
        licenseKeyPath = settings().licensePath();
        minimizeIDE = settings().minimizeIDE();
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
        SquishMessages::toolsInUnexpectedState(m_state, Tr::tr("Refusing to run a test case."));
        return;
    }
    // create test results directory (if necessary) and return on fail
    if (!resultsDirectory.ensureWritableDir()) {
        SquishMessages::criticalMessage(
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
            m_xmlOutputHandler.get(), &SquishXmlOutputHandler::outputAvailable);
    connect(m_xmlOutputHandler.get(), &SquishXmlOutputHandler::updateStatus,
            &m_perspective, &SquishPerspective::updateStatus);

    m_perspective.setPerspectiveMode(SquishPerspective::Running);
    emit squishTestRunStarted();
    setupRunnerForRun();
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
        SquishMessages::toolsInUnexpectedState(m_state, Tr::tr("Refusing to execute server query."));
        return;
    }
    m_perspective.setPerspectiveMode(SquishPerspective::Querying);
    m_query = query;
    setupRunnerForQuery();
    startSquishServer(RunnerQueryRequested);
}

void SquishTools::recordTestCase(const FilePath &suitePath, const QString &testCaseName,
                                 const SuiteConf &suiteConf)
{
    if (m_shutdownInitiated)
        return;
    if (m_state != Idle) {
        SquishMessages::toolsInUnexpectedState(m_state, Tr::tr("Refusing to record a test case."));
        return;
    }

    m_suitePath = suitePath;
    m_testCases = {testCaseName};
    m_suiteConf = suiteConf;

    m_additionalRunnerArguments.clear();
    m_perspective.setPerspectiveMode(SquishPerspective::Recording);
    setupRunnerForRun();
    startSquishServer(RecordTestRequested);
}

void SquishTools::writeServerSettingsChanges(const QList<QStringList> &changes)
{
    if (m_shutdownInitiated)
        return;
    if (m_state != Idle) {
        SquishMessages::toolsInUnexpectedState(m_state, Tr::tr("Refusing to write configuration changes."));
        return;
    }
    m_serverConfigChanges = changes;
    m_perspective.setPerspectiveMode(SquishPerspective::Configuring);
    startSquishServer(ServerConfigChangeRequested);
}

void SquishTools::logAndChangeRunnerState(RunnerState to)
{
    qCInfo(LOG) << "Runner state change:" << runnerStateName(m_squishRunnerState) << ">" << runnerStateName(to);
    m_squishRunnerState = to;
}

void SquishTools::logAndChangeToolsState(SquishTools::State to)
{
    qCInfo(LOG) << "State change:" << toolsStateName(m_state) << ">" << toolsStateName(to);
    m_state = to;
}

void SquishTools::onServerStateChanged(SquishProcessState state)
{
    switch (state) {
    case Starting:
        logAndChangeToolsState(SquishTools::ServerStarting);
        break;
    case Started:
        logAndChangeToolsState(SquishTools::ServerStarted);
        break;
    case StartFailed:
        logAndChangeToolsState(SquishTools::ServerStartFailed);
        onServerStartFailed();
        break;
    case Stopped:
        logAndChangeToolsState(SquishTools::ServerStopped);
        onServerStopped();
        break;
    case StopFailed:
        logAndChangeToolsState(SquishTools::ServerStopFailed);
        onServerStopFailed();
        break;
    default:
        // Idle currently unhandled / not needed?
        break;
    }
}

void SquishTools::onServerPortRetrieved()
{
    QTC_ASSERT(m_state == ServerStarted, return);
    if (m_request == RunnerQueryRequested) {
        executeRunnerQuery();
    } else if (m_request == RunTestRequested || m_request == RecordTestRequested) {
        startSquishRunner();
    } else if (m_request == ServerConfigChangeRequested) { // nothing to do here
    } else {
        QTC_ASSERT(false, qDebug() << m_state << m_request);
    }
}

void SquishTools::onServerStopped()
{
    m_state = Idle;
    emit shutdownFinished();
    if (m_request == ServerConfigChangeRequested) {
        if (m_serverProcess.result() == ProcessResult::FinishedWithError) {
            emit configChangesFailed(m_serverProcess.error());
            return;
        }

        m_serverConfigChanges.removeFirst();
        if (!m_serverConfigChanges.isEmpty()) {
            startSquishServer(ServerConfigChangeRequested);
            return;
        }
        emit configChangesWritten();
        m_perspective.setPerspectiveMode(SquishPerspective::NoMode);
    } else if (m_request == ServerStopRequested) {
        m_request = None;
        if (m_perspective.perspectiveMode() == SquishPerspective::Running)
            emit squishTestRunFinished();

        m_perspective.setPerspectiveMode(SquishPerspective::NoMode);
        if (toolsSettings.minimizeIDE)
            restoreQtCreatorWindows();
        m_perspective.destroyControlBar();
    } else if (m_request == KillOldBeforeQueryRunner) {
        startSquishServer(RunnerQueryRequested);
    } else if (m_request == KillOldBeforeRunRunner) {
        startSquishServer(RunTestRequested);
    } else if (m_request == KillOldBeforeRecordRunner) {
        startSquishServer(RecordTestRequested);
    } else {
        QTC_ASSERT(false, qDebug() << m_request);
    }
}

void SquishTools::onServerStartFailed()
{
    m_state = Idle;
    if (m_request == RunTestRequested)
        emit squishTestRunFinished();
    m_perspective.setPerspectiveMode(SquishPerspective::NoMode);
    m_request = None;
    if (toolsSettings.minimizeIDE)
        restoreQtCreatorWindows();
    m_perspective.destroyControlBar();
}

void SquishTools::onServerStopFailed()
{
    m_serverProcess.closeProcess();
    if (toolsSettings.minimizeIDE)
        restoreQtCreatorWindows();
    m_perspective.destroyControlBar();
    m_state = Idle;
}

void SquishTools::onRunnerStateChanged(SquishProcessState state)
{
    switch (state) {
    case Starting:
        logAndChangeToolsState(SquishTools::RunnerStarting);
        break;
    case Started:
        logAndChangeToolsState(SquishTools::RunnerStarted);
        break;
    case StartFailed:
        logAndChangeToolsState(SquishTools::RunnerStartFailed);
        SquishMessages::criticalMessage(Tr::tr("Squish Runner Error"),
                                        Tr::tr("Squish runner failed to start within given timeframe."));
        onRunnerStopped();
        break;
    case Stopped:
        logAndChangeToolsState(SquishTools::RunnerStopped);
        onRunnerStopped();
    default:
        // rest not needed / handled
        break;
    }
}

void SquishTools::onRunnerStopped()
{
    if (m_request == RunnerQueryRequested) {
        m_request = ServerStopRequested;
        qCInfo(LOG) << "Stopping server from RunnerStopped (query)";
        stopSquishServer();
        return;
    } else if (m_request == RecordTestRequested) {
        if (m_secondaryRunner && m_secondaryRunner->isRunning()) {
            stopRecorder();
        } else {
            m_request = ServerStopRequested;
            qCInfo(LOG) << "Stopping server from RunnerStopped (startaut)";
            stopSquishServer();
        }
        return;
    }
    // below only normal run of test case(s)
    exitAndResetSecondaryRunner();

    if (m_testCases.isEmpty() || (m_squishRunnerState == RunnerState::Canceled)) {
        m_request = ServerStopRequested;
        qCInfo(LOG) << "Stopping server from RunnerStopped";
        stopSquishServer();
        if (QTC_GUARD(m_primaryRunner) && m_primaryRunner->lastRunHadLicenseIssues()) {
            SquishMessages::criticalMessage(Tr::tr("Could not get Squish license from server."));
            return;
        }
        QString error;
        SquishXmlOutputHandler::mergeResultFiles(m_reportFiles,
                                                 m_currentResultsDirectory,
                                                 m_suitePath.fileName(),
                                                 &error);
        if (!error.isEmpty())
            SquishMessages::criticalMessage(error);
        logrotateTestResults();
    } else {
        if (QTC_GUARD(m_primaryRunner) && m_primaryRunner->lastRunHadLicenseIssues()) {
            m_request = ServerStopRequested;
            qCInfo(LOG) << "Stopping server from RunnerStopped (multiple testcases, no license)";
            stopSquishServer();
            SquishMessages::criticalMessage(Tr::tr("Could not get Squish license from server."));
            return;
        }
        m_xmlOutputHandler->clearForNextRun();
        m_perspective.setPerspectiveMode(SquishPerspective::Running);
        logAndChangeRunnerState(RunnerState::Starting);
        startSquishRunner();
    }
}

void SquishTools::onRunnerError(SquishRunnerProcess::RunnerError error)
{
    switch (error) {
    case SquishRunnerProcess::InvalidSocket:
        // we've lost connection to the AUT - if Interrupted, try to cancel the runner
        if (m_squishRunnerState == RunnerState::Interrupted) {
            logAndChangeRunnerState(RunnerState::CancelRequestedWhileInterrupted);
            handlePrompt();
        }
        break;
    case SquishRunnerProcess::MappedAutMissing:
        SquishMessages::criticalMessage(
                    Tr::tr("Squish could not find the AUT \"%1\" to start. Make sure it has been "
                           "added as a Mapped AUT in the squishserver settings.\n"
                           "(Tools > Squish > Server Settings...)").arg(m_suiteConf.aut()));
        break;
    }
}

void SquishTools::setIdle()
{
    m_state = Idle;
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
        const QString detail = Tr::tr("\"%1\" could not be found or is not executable.\nCheck the "
                                      "settings.").arg(toolsSettings.serverPath.toUserOutput());
        SquishMessages::criticalMessage(Tr::tr("Squish Server Error"), detail);
        setIdle();
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
        logAndChangeRunnerState(RunnerState::Starting);
        if (m_request == RecordTestRequested)
            m_perspective.updateStatus(Tr::tr("Recording test case"));
        else
            m_perspective.updateStatus(Tr::tr("Running test case"));
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

    if (m_request == RecordTestRequested)
        m_closeRunnerOnEndRecord = true;

    Utils::CommandLine cmdLine = {toolsSettings.runnerPath, args};
    setupAndStartSquishRunnerProcess(cmdLine);
}

void SquishTools::setupAndStartRecorder()
{
    QTC_ASSERT(m_primaryRunner && m_primaryRunner->autId() != 0, return);
    QTC_ASSERT(!m_secondaryRunner, return);

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
    args << "--autid" << QString::number(m_primaryRunner->autId());

    m_secondaryRunner = new SquishRunnerProcess(this);
    m_secondaryRunner->setupProcess(SquishRunnerProcess::Record);
    const CommandLine cmd = {toolsSettings.runnerPath, args};
    connect(m_secondaryRunner, &SquishRunnerProcess::recorderDone,
            this, &SquishTools::onRecorderFinished);
    qCDebug(LOG) << "Recorder starting:" << cmd.toUserOutput();
    if (m_suiteConf.objectMapPath().isReadableFile())
        Core::DocumentManager::expectFileChange(m_suiteConf.objectMapPath());
    m_secondaryRunner->start(cmd, squishEnvironment());
}

void SquishTools::setupAndStartInspector()
{
    QTC_ASSERT(m_primaryRunner && m_primaryRunner->autId() != 0, return);
    QTC_ASSERT(!m_secondaryRunner, return);

    QStringList args;
    if (!toolsSettings.isLocalServer)
        args << "--host" << toolsSettings.serverHost;
    args << "--port" << QString::number(m_serverProcess.port());
    args << "--debugLog" << "alpw"; // TODO make this configurable?
    args << "--inspect";
    args << "--suitedir" << m_suitePath.toUserOutput();
    args << "--autid" << QString::number(m_primaryRunner->autId());

    m_secondaryRunner = new SquishRunnerProcess(this);
    m_secondaryRunner->setupProcess(SquishRunnerProcess::Inspect);
    const CommandLine cmd = {toolsSettings.runnerPath, args};
    connect(m_secondaryRunner, &SquishRunnerProcess::logOutputReceived,
            this, &SquishTools::logOutputReceived);
    connect(m_secondaryRunner, &SquishRunnerProcess::objectPicked,
            this, &SquishTools::objectPicked);
    connect(m_secondaryRunner, &SquishRunnerProcess::updateChildren,
            this, &SquishTools::updateChildren);
    connect(m_secondaryRunner, &SquishRunnerProcess::propertiesFetched,
            this, &SquishTools::propertiesFetched);
    qCDebug(LOG) << "Inspector starting:" << cmd.toUserOutput();
    m_secondaryRunner->start(cmd, squishEnvironment());
}

void SquishTools::exitAndResetSecondaryRunner()
{
    m_perspective.resetAutId();
    if (m_secondaryRunner) {
        m_secondaryRunner->writeCommand(SquishRunnerProcess::Exit);
        m_secondaryRunner->deleteLater();
        m_secondaryRunner = nullptr;
    }
}

void SquishTools::onInspectTriggered()
{
    QTC_ASSERT(m_primaryRunner, return);
    QTC_ASSERT(m_secondaryRunner, return);
    m_secondaryRunner->writeCommand(SquishRunnerProcess::Pick);
}

void SquishTools::stopRecorder()
{
    QTC_ASSERT(m_secondaryRunner && m_secondaryRunner->isRunning(), return);
    if (m_squishRunnerState == RunnerState::Canceled) {
        qCDebug(LOG) << "Stopping recorder (exit)";
        m_secondaryRunner->writeCommand(SquishRunnerProcess::Exit);
    } else {
        qCDebug(LOG) << "Stopping recorder (endrecord)";
        m_secondaryRunner->writeCommand(SquishRunnerProcess::EndRecord);
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

    QTC_ASSERT(m_primaryRunner, return);
    m_primaryRunner->start(cmdLine, squishEnvironment());
}

Environment SquishTools::squishEnvironment()
{
    Environment environment = Environment::systemEnvironment();
    if (!toolsSettings.licenseKeyPath.isEmpty())
        environment.prependOrSet("SQUISH_LICENSEKEY_DIR", toolsSettings.licenseKeyPath.nativePath());
    environment.prependOrSet("SQUISH_PREFIX", toolsSettings.squishPath.nativePath());
    return environment;
}

void SquishTools::handleQueryDone(const QString &stdOut, const QString &error)
{
    qCDebug(LOG) << "Runner finished";
    if (m_queryCallback)
        m_queryCallback(stdOut, error);
    m_queryCallback = {};
    m_queryParameter.clear();
}

void SquishTools::onRunnerFinished()
{
    qCDebug(LOG) << "Runner finished";
    if (!m_shutdownInitiated) {
        if (m_squishRunnerState == RunnerState::CancelRequested
                || m_squishRunnerState == RunnerState::CancelRequestedWhileInterrupted) {
            logAndChangeRunnerState(RunnerState::Canceled);
        } else {
            logAndChangeRunnerState(RunnerState::Finished);
        }
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
}

void SquishTools::onRecorderFinished()
{
    QTC_ASSERT(m_secondaryRunner, return);
    qCDebug(LOG) << "Recorder finished"; // exit code?
    m_secondaryRunner->deleteLater();
    m_secondaryRunner = nullptr;

    if (m_primaryRunner && m_primaryRunner->isRunning()) {
        if (m_closeRunnerOnEndRecord) {
            //terminateRunner();
            m_primaryRunner->writeCommand(SquishRunnerProcess::Exit); // why doesn't work anymore?
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
            logAndChangeRunnerState(RunnerState::Canceled);
            stopRecorder();
            break;
        case RunnerState::Canceled:
            QTC_CHECK(false);
            break;
        default:
            break;
        }
        return;
    }

    QTC_ASSERT(m_primaryRunner, return);
    switch (m_squishRunnerState) {
    case RunnerState::Starting: {
        const Utils::Links setBPs = m_primaryRunner->setBreakpoints(m_suiteConf.scriptExtension());
        if (!setBPs.contains({Utils::FilePath::fromUserInput(fileName), line})) {
            onRunnerRunRequested(StepMode::Continue);
        } else {
            m_perspective.setPerspectiveMode(SquishPerspective::Interrupted);
            logAndChangeRunnerState(RunnerState::Interrupted);
            restoreQtCreatorWindows();
            // request local variables
            m_primaryRunner->writeCommand(SquishRunnerProcess::PrintVariables);
            const FilePath filePath = FilePath::fromUserInput(fileName);
            Core::EditorManager::openEditorAt({filePath, line, column});
            updateLocationMarker(filePath, line);
        }
        break;
    }
    case RunnerState::CancelRequested:
    case RunnerState::CancelRequestedWhileInterrupted:
        logAndChangeRunnerState(RunnerState::Canceled);
        exitAndResetSecondaryRunner();
        m_primaryRunner->writeCommand(SquishRunnerProcess::Exit);
        clearLocationMarker();
        break;
    case RunnerState::Canceled:
        QTC_CHECK(false);
        break;
    default:
        if (line != -1 && column != -1) {
            m_perspective.setPerspectiveMode(SquishPerspective::Interrupted);
            logAndChangeRunnerState(RunnerState::Interrupted);
            restoreQtCreatorWindows();
            // if we're returning from a function we might end up without a file information
            if (fileName.isEmpty()) {
                m_primaryRunner->writeCommand(SquishRunnerProcess::Next);
            } else {
                // request local variables
                m_primaryRunner->writeCommand(SquishRunnerProcess::PrintVariables);
                const FilePath filePath = FilePath::fromUserInput(fileName);
                Core::EditorManager::openEditorAt({filePath, line, column});
                updateLocationMarker(filePath, line);
                // looks like we need to start inspector while being interrupted?
                if (!m_secondaryRunner && m_primaryRunner->autId() !=0)
                    setupAndStartInspector();
            }
        } else { // it's just some output coming from the server
            if (m_squishRunnerState == RunnerState::Interrupted && !m_requestVarsTimer) {
                // FIXME: this should be easier, but when interrupted and AUT is closed
                // runner does not get notified until continued/canceled
                m_requestVarsTimer = new QTimer(this);
                m_requestVarsTimer->setSingleShot(true);
                m_requestVarsTimer->setInterval(1000);
                connect(m_requestVarsTimer, &QTimer::timeout, this, [this] {
                    m_primaryRunner->writeCommand(SquishRunnerProcess::PrintVariables);
                });
                m_requestVarsTimer->start();
            }
        }
    }
}

void SquishTools::requestExpansion(const QString &name)
{
    QTC_ASSERT(m_primaryRunner, return);
    QTC_ASSERT(m_squishRunnerState == RunnerState::Interrupted, return);
    m_primaryRunner->requestExpanded(name);
}

void SquishTools::requestExpansionForObject(const QString &value)
{
    QTC_ASSERT(m_primaryRunner, return);
    if (m_squishRunnerState != RunnerState::Interrupted)
        return;
    QTC_ASSERT(m_secondaryRunner, return);
    m_secondaryRunner->requestListObject(value);
}

void SquishTools::requestPropertiesForObject(const QString &value)
{
    QTC_ASSERT(m_primaryRunner, return);
    if (m_squishRunnerState != RunnerState::Interrupted)
        return;
    QTC_ASSERT(m_secondaryRunner, return);
    m_secondaryRunner->requestListProperties(value);
}

bool SquishTools::shutdown()
{
    QTC_ASSERT(!m_shutdownInitiated, return true);
    m_shutdownInitiated = true;
    if (m_primaryRunner && m_primaryRunner->isRunning())
        terminateRunner();
    if (m_serverProcess.isRunning())
        m_serverProcess.stop();
    return !(m_serverProcess.isRunning() || (m_primaryRunner && m_primaryRunner->isRunning()));
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
        m_locationMarker->updateFilePath(file);
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
    logAndChangeRunnerState(RunnerState::RunRequested);

    QTC_ASSERT(m_primaryRunner, return);
    if (step == StepMode::Continue)
        m_primaryRunner->writeCommand(SquishRunnerProcess::Continue);
    else if (step == StepMode::StepIn)
        m_primaryRunner->writeCommand(SquishRunnerProcess::Step);
    else if (step == StepMode::StepOver)
        m_primaryRunner->writeCommand(SquishRunnerProcess::Next);
    else if (step == StepMode::StepOut)
        m_primaryRunner->writeCommand(SquishRunnerProcess::Return);

    clearLocationMarker();
    if (toolsSettings.minimizeIDE)
        minimizeQtCreatorWindows();
    // avoid overriding Recording
    if (m_perspective.perspectiveMode() == SquishPerspective::Interrupted)
        m_perspective.setPerspectiveMode(SquishPerspective::Running);

    logAndChangeRunnerState(RunnerState::Running);
}

void SquishTools::interruptRunner()
{
    qCDebug(LOG) << "Interrupting runner";
    QTC_ASSERT(m_primaryRunner, return);
    qint64 processId = m_primaryRunner->processId();
    const CommandLine cmd(toolsSettings.processComPath, {QString::number(processId), "break"});
    Process process;
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
    QTC_ASSERT(m_primaryRunner, return);
    qint64 processId = m_primaryRunner->processId();
    const CommandLine cmd(toolsSettings.processComPath, {QString::number(processId), "terminate"});
    Process process;
    process.setCommand(cmd);
    process.start();
    process.waitForFinished();
}

void SquishTools::handleSquishServerAlreadyRunning()
{
    const QString detail = Tr::tr("There is still an old Squish server instance running.\n"
                                  "This will cause problems later on.\n\n"
                                  "If you continue, the old instance will be terminated.\n"
                                  "Do you want to continue?");
    if (SquishMessages::simpleQuestion(Tr::tr("Squish Server Already Running"), detail) != QMessageBox::Yes)
        return;

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
        const QString detail = Tr::tr("Unexpected state or request while starting Squish server. "
                                      "(state: %1, request: %2)").arg(m_state).arg(m_request);
        SquishMessages::criticalMessage(detail);
    }
    stopSquishServer();
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
        const QString detail = Tr::tr("Squish server does not seem to be running.\n(state: %1, "
                                      "request: %2)\nTry again.").arg(m_state).arg(m_request);
        SquishMessages::criticalMessage(Tr::tr("No Squish Server"), detail);
        setIdle();
        return false;
    }
    if (m_serverProcess.port() == -1) {
        const QString detail = Tr::tr("Failed to get the server port.\n(state: %1, request: %2)\n"
                                      "Try again.").arg(m_state).arg(m_request);
        SquishMessages::criticalMessage(Tr::tr("No Squish Server Port"), detail);
        // setting state to ServerStartFailed will terminate/kill the current unusable server
        onServerStateChanged(StartFailed);
        return false;
    }

    if (m_primaryRunner && m_primaryRunner->state() != QProcess::NotRunning) {
        const QString detail = Tr::tr("Squish runner seems to be running already.\n(state: %1, "
                                      "request: %2)\nWait until it has finished and try again.")
                .arg(m_state).arg(m_request);
        SquishMessages::criticalMessage(Tr::tr("Squish Runner Running"), detail);
        return false;
    }
    return true;
}

bool SquishTools::setupRunnerPath()
{
    const FilePath squishRunner = Environment::systemEnvironment().searchInPath(
        toolsSettings.runnerPath.toString());
    if (!squishRunner.isExecutableFile()) {
        const QString detail = Tr::tr("\"%1\" could not be found or is not executable.\nCheck the "
                                      "settings.").arg(toolsSettings.runnerPath.toUserOutput());
        SquishMessages::criticalMessage(Tr::tr("Squish Runner Error"), detail);
        onRunnerStateChanged(Stopped);
        return false;
    }
    toolsSettings.runnerPath = squishRunner;
    return true;
}

void SquishTools::setupAndStartSquishRunnerProcess(const Utils::CommandLine &cmdLine)
{
    QTC_ASSERT(m_primaryRunner, return);
    // avoid crashes on fast re-usage of Process
    m_primaryRunner->closeProcess();

    if (m_request == RunTestRequested) {
        connect(m_primaryRunner, &SquishRunnerProcess::autIdRetrieved,
                this, &SquishTools::autIdRetrieved);
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

    m_primaryRunner->setTestCasePath(m_currentTestCasePath);
    m_primaryRunner->start(cmdLine, squishEnvironment());
}

void SquishTools::setupRunnerForQuery()
{
    if (m_primaryRunner)
        delete m_primaryRunner;

    m_primaryRunner = new SquishRunnerProcess(this);
    m_primaryRunner->setupProcess(SquishRunnerProcess::QueryServer);
    connect(m_primaryRunner, &SquishRunnerProcess::queryDone,
            this, &SquishTools::handleQueryDone);
    connect(m_primaryRunner, &SquishRunnerProcess::stateChanged,
            this, &SquishTools::onRunnerStateChanged);
    connect(m_primaryRunner, &SquishRunnerProcess::logOutputReceived,
            this, &SquishTools::logOutputReceived);
}

void SquishTools::setupRunnerForRun()
{
    if (m_primaryRunner)
        delete m_primaryRunner;

    m_primaryRunner = new SquishRunnerProcess(this);
    m_primaryRunner->setupProcess(m_request == RecordTestRequested ? SquishRunnerProcess::StartAut
                                                                   : SquishRunnerProcess::Run);
    connect(m_primaryRunner, &SquishRunnerProcess::interrupted,
            this, &SquishTools::handlePrompt);
    connect(m_primaryRunner, &SquishRunnerProcess::localsUpdated,
            this, &SquishTools::localsUpdated);
    connect(m_primaryRunner, &SquishRunnerProcess::runnerFinished,
            this, &SquishTools::onRunnerFinished);
    connect(m_primaryRunner, &SquishRunnerProcess::runnerError,
            this, &SquishTools::onRunnerError);
    connect(m_primaryRunner, &SquishRunnerProcess::stateChanged,
            this, &SquishTools::onRunnerStateChanged);
    connect(m_primaryRunner, &SquishRunnerProcess::logOutputReceived,
            this, &SquishTools::logOutputReceived);
}

} // namespace Squish::Internal
