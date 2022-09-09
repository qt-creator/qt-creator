// Copyright (C) 2022 The Qt Company Ltd
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "squishperspective.h"

#include <utils/environment.h>
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
    void runTestCases(const QString &suitePath,
                      const QStringList &testCases = QStringList(),
                      const QStringList &additionalServerArgs = QStringList(),
                      const QStringList &additionalRunnerArgs = QStringList());
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
    void handleSetStateQueryRunner();
    void setIdle();
    void startSquishServer(Request request);
    void stopSquishServer();
    void startSquishRunner();
    void executeRunnerQuery();
    static Utils::Environment squishEnvironment();
    void onServerFinished();
    void onRunnerFinished();
    void onServerOutput();
    void onServerErrorOutput();
    void onRunnerOutput();                              // runner's results file
    void onRunnerErrorOutput();                         // runner's error stream
    void onRunnerStdOutput(const QString &line);        // runner's output stream
    void setBreakpoints();
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
    bool setupRunnerPath();
    void setupAndStartSquishRunnerProcess(const QStringList &arg,
                                          const QString &caseReportFilePath = {});

    SquishPerspective m_perspective;
    std::unique_ptr<SquishXmlOutputHandler> m_xmlOutputHandler;
    Utils::QtcProcess m_serverProcess;
    Utils::QtcProcess m_runnerProcess;
    int m_serverPort = -1;
    QString m_serverHost;
    Request m_request = None;
    State m_state = Idle;
    RunnerState m_squishRunnerState = RunnerState::None;
    QString m_suitePath;
    QStringList m_testCases;
    QStringList m_reportFiles;
    QString m_currentResultsDirectory;
    QString m_fullRunnerOutput; // used when querying the server
    Utils::FilePath m_currentTestCasePath;
    QFile *m_currentResultsXML = nullptr;
    QFileSystemWatcher *m_resultsFileWatcher = nullptr;
    QStringList m_additionalServerArguments;
    QStringList m_additionalRunnerArguments;
    QList<QStringList> m_serverConfigChanges;
    QWindowList m_lastTopLevelWindows;
    class SquishLocationMark *m_locationMarker = nullptr;
    QTimer *m_requestVarsTimer = nullptr;
    qint64 m_readResultsCount;
    bool m_shutdownInitiated = false;
};

} // namespace Internal
} // namespace Squish
