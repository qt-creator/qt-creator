// Copyright (C) 2022 The Qt Company Ltd
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "squishperspective.h"
#include "squishrunnerprocess.h"
#include "squishserverprocess.h"
#include "suiteconf.h"

#include <utils/environment.h>
#include <utils/process.h>

#include <QObject>
#include <QStringList>

#include <memory>

QT_BEGIN_NAMESPACE
class QFile;
class QFileSystemWatcher;
QT_END_NAMESPACE

namespace Squish {
namespace Internal {

class SquishXmlOutputHandler;

class SquishTools : public QObject
{
    Q_OBJECT
public:
    explicit SquishTools(QObject *parent = nullptr);
    ~SquishTools() override;

    static SquishTools *instance();

    enum State {
        Idle,
        ServerStarting,
        ServerStarted,
        ServerStartFailed,
        ServerStopped,
        ServerStopFailed,
        RunnerStarting,
        RunnerStarted,
        RunnerStartFailed,
        RunnerStopped
    };

    using QueryCallback = std::function<void(const QString &, const QString &)>;

    State state() const { return m_state; }
    void runTestCases(const Utils::FilePath &suitePath,
                      const QStringList &testCases = QStringList());
    void recordTestCase(const Utils::FilePath &suitePath, const QString &testCaseName,
                        const SuiteConf &suiteConf);
    void queryGlobalScripts(QueryCallback callback);
    void queryServerSettings(QueryCallback callback);
    void requestSetSharedFolders(const Utils::FilePaths &sharedFolders);
    void writeServerSettingsChanges(const QList<QStringList> &changes);
    void requestExpansion(const QString &name);
    void requestExpansionForObject(const QString &value);
    void requestPropertiesForObject(const QString &value);

    bool shutdown();

signals:
    void autIdRetrieved();
    void logOutputReceived(const QString &output);
    void squishTestRunStarted();
    void squishTestRunFinished();
    void resultOutputCreated(const QByteArray &output);
    void configChangesFailed(QProcess::ProcessError error);
    void configChangesWritten();
    void localsUpdated(const QString &output);
    void objectPicked(const QString &output);
    void updateChildren(const QString &realName, const QStringList &children);
    void propertiesFetched(const QStringList &properties);
    void shutdownFinished();

private:
    enum Request {
        None,
        ServerStopRequested,
        ServerConfigChangeRequested,
        RunnerQueryRequested,
        RunTestRequested,
        RecordTestRequested,
        KillOldBeforeRunRunner,
        KillOldBeforeRecordRunner,
        KillOldBeforeQueryRunner
    };

    enum RunnerQuery { ServerInfo, GetGlobalScriptDirs, SetGlobalScriptDirs };

    void logAndChangeRunnerState(RunnerState to);
    void logAndChangeToolsState(SquishTools::State to);
    void onServerStateChanged(SquishProcessState state);
    void onServerPortRetrieved();
    void onServerStopped();
    void onServerStartFailed();
    void onServerStopFailed();
    void onRunnerStateChanged(SquishProcessState state);
    void onRunnerStopped();
    void onRunnerError(SquishRunnerProcess::RunnerError error);
    void setIdle();
    void startSquishServer(Request request);
    void stopSquishServer();
    void startSquishRunner();
    void setupAndStartRecorder();
    void setupAndStartInspector();
    void exitAndResetSecondaryRunner();
    void onInspectTriggered();
    void stopRecorder();
    void queryServer(RunnerQuery query);
    void executeRunnerQuery();
    static Utils::Environment squishEnvironment();
    void handleQueryDone(const QString &stdOut, const QString &error);
    void onRunnerFinished();
    void onRecorderFinished();
    void onRunnerOutput();                              // runner's results file
    void handlePrompt(const QString &fileName = {}, int line = -1, int column = -1);
    void onResultsDirChanged(const QString &filePath);
    static void logrotateTestResults();
    void minimizeQtCreatorWindows();
    void restoreQtCreatorWindows();
    void updateLocationMarker(const Utils::FilePath &file, int line);
    void clearLocationMarker();
    void onRunnerRunRequested(StepMode step);
    void interruptRunner();
    void terminateRunner();
    bool isValidToStartRunner();
    void handleSquishServerAlreadyRunning();
    QStringList serverArgumentsFromSettings() const;
    QStringList runnerArgumentsFromSettings();
    bool setupRunnerPath();
    void setupAndStartSquishRunnerProcess(const Utils::CommandLine &cmdLine);
    void setupRunnerForQuery();
    void setupRunnerForRun();

    SquishPerspective m_perspective;
    std::unique_ptr<SquishXmlOutputHandler> m_xmlOutputHandler;
    SquishServerProcess m_serverProcess;

    SquishRunnerProcess *m_primaryRunner = nullptr;
    SquishRunnerProcess *m_secondaryRunner = nullptr;

    QString m_serverHost;
    Request m_request = None;
    State m_state = Idle;
    RunnerState m_squishRunnerState = RunnerState::None;
    Utils::FilePath m_suitePath;
    QStringList m_testCases;
    SuiteConf m_suiteConf; // holds information of current test suite e.g. while recording
    Utils::FilePaths m_reportFiles;
    Utils::FilePath m_currentResultsDirectory;
    QString m_queryParameter;
    Utils::FilePath m_currentTestCasePath;
    Utils::FilePath m_currentRecorderSnippetFile;
    QFile *m_currentResultsXML = nullptr;
    QFileSystemWatcher *m_resultsFileWatcher = nullptr;
    QStringList m_additionalRunnerArguments;
    QList<QStringList> m_serverConfigChanges;
    QWindowList m_lastTopLevelWindows;
    class SquishLocationMark *m_locationMarker = nullptr;
    QTimer *m_requestVarsTimer = nullptr;
    qint64 m_readResultsCount;
    QueryCallback m_queryCallback;
    RunnerQuery m_query = ServerInfo;
    bool m_shutdownInitiated = false;
    bool m_closeRunnerOnEndRecord = false;
};

} // namespace Internal
} // namespace Squish
