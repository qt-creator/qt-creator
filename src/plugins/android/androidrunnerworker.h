// Copyright (C) 2018 BogDan Vatra <bog_dan_ro@yahoo.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


#pragma once

#include <qmldebug/qmldebugcommandlinearguments.h>

#include <solutions/tasking/tasktreerunner.h>

#include <utils/environment.h>
#include <utils/port.h>

namespace Android { class AndroidDeviceInfo; }
namespace ProjectExplorer { class RunWorker; }
namespace Utils { class Process; }

namespace Android::Internal {

using PidUserPair = std::pair<qint64, qint64>;

class AndroidRunnerWorker : public QObject
{
    Q_OBJECT
public:
    AndroidRunnerWorker(ProjectExplorer::RunWorker *runner);
    ~AndroidRunnerWorker() override;

    void setAndroidDeviceInfo(const AndroidDeviceInfo &info);
    void asyncStart();
    void asyncStop();

signals:
    void remoteProcessStarted(Utils::Port debugServerPort, const QUrl &qmlServer, qint64 pid);
    void remoteProcessFinished(const QString &errString = QString());

    void remoteOutput(const QString &output);
    void remoteErrorOutput(const QString &output);

private:
    bool runAdb(const QStringList &args, QString *stdOut = nullptr, QString *stdErr = nullptr);
    QStringList selector() const;
    void forceStop();
    void logcatReadStandardError();
    void logcatReadStandardOutput();
    void logcatProcess(const QByteArray &text, QByteArray &buffer, bool onlyError);

    void handleJdbWaiting();
    void handleJdbSettled();

    bool removeForwardPort(const QString &port, const QString &adbArg, const QString &portType);

    void asyncStartHelper();
    void startNativeDebugging();
    void startDebuggerServer(const QString &packageDir, const QString &debugServerFile);
    bool deviceFileExists(const QString &filePath);
    bool packageFileExists(const QString& filePath);
    bool uploadDebugServer(const QString &debugServerFileName);
    void asyncStartLogcat();

    enum class JDBState {
        Idle,
        Waiting,
        Settled
    };
    void onProcessIdChanged(const PidUserPair &pidUser);
    bool isPreNougat() const { return m_apiLevel > 0 && m_apiLevel <= 23; }
    Tasking::ExecutableItem removeForwardPortRecipe(const QString &port, const QString &adbArg,
                                                    const QString &portType);
    Tasking::ExecutableItem preStartRecipe();
    Tasking::ExecutableItem pidRecipe();

    // Create the processes and timer in the worker thread, for correct thread affinity
    QString m_packageName;
    QString m_intentName;
    QStringList m_beforeStartAdbCommands;
    QStringList m_afterFinishAdbCommands;
    QStringList m_amStartExtraArgs;
    qint64 m_processPID = -1;
    qint64 m_processUser = -1;
    std::unique_ptr<Utils::Process> m_adbLogcatProcess;
    std::unique_ptr<Utils::Process> m_psIsAlive;
    QByteArray m_stdoutBuffer;
    QByteArray m_stderrBuffer;
    Tasking::TaskTreeRunner m_taskTreeRunner;
    bool m_useCppDebugger = false;
    bool m_useLldb = false; // FIXME: Un-implemented currently.
    QmlDebug::QmlDebugServicesPreset m_qmlDebugServices;
    QUrl m_qmlServer;
    JDBState m_jdbState = JDBState::Idle;
    std::unique_ptr<Utils::Process> m_debugServerProcess; // gdbserver or lldb-server
    std::unique_ptr<Utils::Process> m_jdbProcess;
    QString m_deviceSerialNumber;
    int m_apiLevel = -1;
    QString m_extraAppParams;
    Utils::Environment m_extraEnvVars;
    Utils::FilePath m_debugServerPath; // On build device, typically as part of ndk
    bool m_useAppParamsForQmlDebugger = false;
};

} // namespace Android::Internal
