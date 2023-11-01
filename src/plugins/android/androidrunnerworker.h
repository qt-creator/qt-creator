// Copyright (C) 2018 BogDan Vatra <bog_dan_ro@yahoo.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


#pragma once

#include <qmldebug/qmldebugcommandlinearguments.h>

#include <utils/environment.h>
#include <utils/port.h>

#include <QFuture>
#include <utility>

namespace Utils {
class FilePath;
class Process;
}
namespace ProjectExplorer { class RunWorker; }

namespace Android {

class AndroidDeviceInfo;

namespace Internal {

const int MIN_SOCKET_HANDSHAKE_PORT = 20001;

using PidUserPair = std::pair<qint64, qint64>;

class AndroidRunnerWorker : public QObject
{
    Q_OBJECT
public:
    AndroidRunnerWorker(ProjectExplorer::RunWorker *runner, const QString &packageName);
    ~AndroidRunnerWorker() override;

    bool runAdb(const QStringList &args, QString *stdOut = nullptr, QString *stdErr = nullptr,
                const QByteArray &writeData = {});
    void adbKill(qint64 pid);
    QStringList selector() const;
    void forceStop();
    void logcatReadStandardError();
    void logcatReadStandardOutput();
    void logcatProcess(const QByteArray &text, QByteArray &buffer, bool onlyError);
    void setAndroidDeviceInfo(const AndroidDeviceInfo &info);
    void setIsPreNougat(bool isPreNougat) { m_isPreNougat = isPreNougat; }
    void setIntentName(const QString &intentName) { m_intentName = intentName; }

    void asyncStart();
    void asyncStop();
    void handleJdbWaiting();
    void handleJdbSettled();

    void removeForwardPort(const QString &port);

signals:
    void remoteProcessStarted(Utils::Port debugServerPort, const QUrl &qmlServer, qint64 pid);
    void remoteProcessFinished(const QString &errString = QString());

    void remoteOutput(const QString &output);
    void remoteErrorOutput(const QString &output);

private:
    void asyncStartHelper();
    void startNativeDebugging();
    bool startDebuggerServer(const QString &packageDir, const QString &debugServerFile, QString *errorStr = nullptr);
    bool deviceFileExists(const QString &filePath);
    bool packageFileExists(const QString& filePath);
    bool uploadDebugServer(const QString &debugServerFileName);
    void asyncStartLogcat();

    enum class JDBState {
        Idle,
        Waiting,
        Settled
    };
    void onProcessIdChanged(PidUserPair pidUser);

    // Create the processes and timer in the worker thread, for correct thread affinity
    bool m_isPreNougat = false;
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
    QFuture<PidUserPair> m_pidFinder;
    bool m_useCppDebugger = false;
    bool m_useLldb = false; // FIXME: Un-implemented currently.
    QmlDebug::QmlDebugServicesPreset m_qmlDebugServices;
    Utils::Port m_localDebugServerPort; // Local end of forwarded debug socket.
    QUrl m_qmlServer;
    JDBState m_jdbState = JDBState::Idle;
    Utils::Port m_localJdbServerPort;
    std::unique_ptr<Utils::Process> m_debugServerProcess; // gdbserver or lldb-server
    std::unique_ptr<Utils::Process> m_jdbProcess;
    QString m_deviceSerialNumber;
    int m_apiLevel = -1;
    QString m_extraAppParams;
    Utils::Environment m_extraEnvVars;
    Utils::FilePath m_debugServerPath; // On build device, typically as part of ndk
    bool m_useAppParamsForQmlDebugger = false;
};

} // namespace Internal
} // namespace Android
