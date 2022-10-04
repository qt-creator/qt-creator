// Copyright (C) 2022 The Qt Company Ltd
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "squishperspective.h"
#include "suiteconf.h"

#include <utils/environment.h>
#include <utils/link.h>
#include <utils/qtcprocess.h>

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

    enum class RunnerState {
        None,
        Starting,
        Running,
        RunRequested,
        Interrupted,
        InterruptRequested,
        Canceling,
        Canceled,
        CancelRequested,
        CancelRequestedWhileInterrupted,
        Finished
    };

    State state() const { return m_state; }
    void runTestCases(const Utils::FilePath &suitePath,
                      const QStringList &testCases = QStringList());
    void recordTestCase(const Utils::FilePath &suitePath, const QString &testCaseName,
                        const SuiteConf &suiteConf);
    void queryServerSettings();
    void writeServerSettingsChanges(const QList<QStringList> &changes);
    void requestExpansion(const QString &name);

    bool shutdown();

signals:
    void logOutputReceived(const QString &output);
    void squishTestRunStarted();
    void squishTestRunFinished();
    void resultOutputCreated(const QByteArray &output);
    void queryFinished(const QString &output);
    void configChangesFailed(QProcess::ProcessError error);
    void configChangesWritten();
    void localsUpdated(const QString &output);
    void symbolUpdated(const QString &output);
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

    void setState(State state);
    void handleSetStateStartAppRunner();
    void handleSetStateQueryRunner();
    void setIdle();
    void startSquishServer(Request request);
    void stopSquishServer();
    void startSquishRunner();
    void setupAndStartRecorder();
    void stopRecorder();
    void executeRunnerQuery();
    static Utils::Environment squishEnvironment();
    void onServerFinished();
    void onRunnerFinished();
    void onRecorderFinished();
    void onServerOutput();
    void onServerErrorOutput();
    void onRunnerOutput();                              // runner's results file
    void onRunnerErrorOutput();                         // runner's error stream
    void onRunnerStdOutput(const QString &line);        // runner's output stream
    Utils::Links setBreakpoints();
    void handlePrompt(const QString &fileName = {}, int line = -1, int column = -1);
    void onResultsDirChanged(const QString &filePath);
    static void logrotateTestResults();
    void minimizeQtCreatorWindows();
    void restoreQtCreatorWindows();
    void updateLocationMarker(const Utils::FilePath &file, int line);
    void clearLocationMarker();
    void onRunnerRunRequested(SquishPerspective::StepMode step);
    void interruptRunner();
    void terminateRunner();
    bool isValidToStartRunner();
    void handleSquishServerAlreadyRunning();
    QStringList serverArgumentsFromSettings() const;
    QStringList runnerArgumentsFromSettings();
    bool setupRunnerPath();
    void setupAndStartSquishRunnerProcess(const QStringList &arg);

    SquishPerspective m_perspective;
    std::unique_ptr<SquishXmlOutputHandler> m_xmlOutputHandler;
    Utils::QtcProcess m_serverProcess;
    Utils::QtcProcess m_runnerProcess;
    Utils::QtcProcess m_recorderProcess;
    int m_serverPort = -1;
    QString m_serverHost;
    Request m_request = None;
    State m_state = Idle;
    RunnerState m_squishRunnerState = RunnerState::None;
    Utils::FilePath m_suitePath;
    QStringList m_testCases;
    SuiteConf m_suiteConf; // holds information of current test suite e.g. while recording
    Utils::FilePaths m_reportFiles;
    Utils::FilePath m_currentResultsDirectory;
    QString m_fullRunnerOutput; // used when querying the server
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
    int m_autId = 0;
    bool m_shutdownInitiated = false;
    bool m_closeRunnerOnEndRecord = false;
};

} // namespace Internal
} // namespace Squish
