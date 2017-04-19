/****************************************************************************
**
** Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "androidrunner.h"

#include "androiddeployqtstep.h"
#include "androidconfigurations.h"
#include "androidglobal.h"
#include "androidrunconfiguration.h"
#include "androidmanager.h"
#include "androidavdmanager.h"

#include <debugger/debuggerrunconfigurationaspect.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectexplorersettings.h>
#include <projectexplorer/target.h>
#include <qtsupport/qtkitinformation.h>
#include <utils/qtcassert.h>
#include <utils/runextensions.h>
#include <utils/synchronousprocess.h>
#include <utils/temporaryfile.h>

#include <chrono>
#include <memory>
#include <QApplication>
#include <QDir>
#include <QRegExp>
#include <QTime>
#include <QTcpServer>
#include <QTcpSocket>
#include <QRegularExpression>

using namespace std;
using namespace std::placeholders;
using namespace ProjectExplorer;

/*
    This uses explicit handshakes between the application and the
    gdbserver start and the host side by using the gdbserver socket.

    For the handshake there are two mechanisms. Only the first method works
    on Android 5.x devices and is chosen as default option. The second
    method can be enabled by setting the QTC_ANDROID_USE_FILE_HANDSHAKE
    environment variable before starting Qt Creator.

    1.) This method uses a TCP server on the Android device which starts
    listening for incoming connections. The socket is forwarded by adb
    and creator connects to it. This is the only method that works
    on Android 5.x devices.

    2.) This method uses two files ("ping" file in the application dir,
    "pong" file in /data/local/tmp/qt).

    The sequence is as follows:

     host: adb forward debugsocket :5039

     host: adb shell rm pong file
     host: adb shell am start
     host: loop until ping file appears

         app start up: launch gdbserver --multi +debug-socket
         app start up: loop until debug socket appear

             gdbserver: normal start up including opening debug-socket,
                        not yet attached to any process

         app start up: 1.) set up ping connection or 2.) touch ping file
         app start up: 1.) accept() or 2.) loop until pong file appears

     host: start gdb
     host: gdb: set up binary, breakpoints, path etc
     host: gdb: target extended-remote :5039

             gdbserver: accepts connection from gdb

     host: gdb: attach <application-pid>

             gdbserver: attaches to the application
                        and stops it

         app start up: stopped now (it is still waiting for
                       the pong anyway)

     host: gdb: continue

             gdbserver: resumes application

         app start up: resumed (still waiting for the pong)

     host: 1) write "ok" to ping pong connection or 2.) write pong file

         app start up: java code continues now, the process
                       is already fully under control
                       of gdbserver. Breakpoints are set etc,
                       we are before main.
         app start up: native code launches

*/

namespace Android {
namespace Internal {

const int MIN_SOCKET_HANDSHAKE_PORT = 20001;
const int MAX_SOCKET_HANDSHAKE_PORT = 20999;
static const QString pidScript = QStringLiteral("input keyevent KEYCODE_WAKEUP; "
                                                "while true; do sleep 1; echo \"=\"; "
                                                "for p in /proc/[0-9]*; "
                                                "do cat <$p/cmdline && echo :${p##*/}; done; done");

static const QString regExpLogcat = QStringLiteral("[0-9\\-]*"  // date
                                                  "\\s+"
                                                  "[0-9\\-:.]*"// time
                                                  "\\s*"
                                                  "(\\d*)"     // pid           1. capture
                                                  "\\s+"
                                                  "\\d*"       // unknown
                                                  "\\s+"
                                                  "(\\w)"      // message type  2. capture
                                                  "\\s+"
                                                  "(.*): "     // source        3. capture
                                                  "(.*)"       // message       4. capture
                                                  "[\\n\\r]*"
                                                 );
static int APP_START_TIMEOUT = 45000;

enum class PidStatus {
    Found,
    Lost
};

struct PidInfo
{
    PidInfo(qint64 pid = -1, PidStatus status = PidStatus::Lost, QString name = {})
        : pid(pid)
        , status(status)
        , name(name)
    {}
    qint64 pid;
    PidStatus status;
    QString name;
};

static void deleter(QProcess *p)
{
    p->disconnect();
    p->kill();
    p->waitForFinished();
    // Might get deleted from its own signal handler.
    p->deleteLater();
}

class AndroidRunnerWorker : public QObject
{
    Q_OBJECT

    enum DebugHandShakeType {
        PingPongFiles,
        SocketHandShake
    };

public:
    AndroidRunnerWorker(AndroidRunConfiguration *runConfig, Core::Id runMode,
                        const QString &packageName, const QStringList &selector);
    ~AndroidRunnerWorker();

    void asyncStart(const QString &intentName, const QVector<QStringList> &adbCommands);
    void asyncStop(const QVector<QStringList> &adbCommands);

    void setAdbParameters(const QString &packageName, const QStringList &selector);
    void handleRemoteDebuggerRunning();

signals:
    void remoteServerRunning(const QByteArray &serverChannel, int pid);
    void remoteProcessStarted(Utils::Port gdbServerPort, Utils::Port qmlPort);
    void remoteProcessFinished(const QString &errString = QString());

    void remoteOutput(const QString &output);
    void remoteErrorOutput(const QString &output);
    void pidFound(qint64, const QString &name);
    void pidLost(qint64);

private:
    void findProcessPids();
    void onProcessIdChanged(PidInfo pidInfo);
    void logcatReadStandardError();
    void logcatReadStandardOutput();
    void adbKill(qint64 pid);
    QStringList selector() const { return m_selector; }
    void forceStop();
    void logcatProcess(const QByteArray &text, QByteArray &buffer, bool onlyError);
    bool adbShellAmNeedsQuotes();
    bool runAdb(const QStringList &args, QString *exitMessage = nullptr, int timeoutS = 10);
    int deviceSdkVersion();

    // Create the processes and timer in the worker thread, for correct thread affinity
    std::unique_ptr<QProcess, decltype(&deleter)> m_adbLogcatProcess;
    std::unique_ptr<QProcess, decltype(&deleter)> m_pidsFinderProcess;
    QScopedPointer<QTcpSocket> m_socket;

    QByteArray m_stdoutBuffer;
    QByteArray m_stderrBuffer;

    QSet<qint64> m_processPids;
    bool m_useCppDebugger = false;
    QmlDebug::QmlDebugServicesPreset m_qmlDebugServices;
    Utils::Port m_localGdbServerPort; // Local end of forwarded debug socket.
    Utils::Port m_qmlPort;
    QString m_pingFile;
    QString m_pongFile;
    QString m_gdbserverPath;
    QString m_gdbserverSocket;
    QString m_adb;
    QStringList m_selector;
    DebugHandShakeType m_handShakeMethod = SocketHandShake;
    bool m_customPort = false;

    QString m_packageName;
    int m_socketHandShakePort = MIN_SOCKET_HANDSHAKE_PORT;
    QByteArray m_pidsBuffer;
    QScopedPointer<QTimer> m_timeoutTimer;
};

AndroidRunnerWorker::AndroidRunnerWorker(AndroidRunConfiguration *runConfig, Core::Id runMode,
                                           const QString &packageName, const QStringList &selector)
    : m_adbLogcatProcess(nullptr, deleter)
    , m_pidsFinderProcess(nullptr, deleter)
    , m_selector(selector)
    , m_packageName(packageName)
{
    Debugger::DebuggerRunConfigurationAspect *aspect
            = runConfig->extraAspect<Debugger::DebuggerRunConfigurationAspect>();
    const bool debuggingMode =
            (runMode == ProjectExplorer::Constants::DEBUG_RUN_MODE
             || runMode == ProjectExplorer::Constants::DEBUG_RUN_MODE_WITH_BREAK_ON_MAIN);
    m_useCppDebugger = debuggingMode && aspect->useCppDebugger();
    if (debuggingMode && aspect->useQmlDebugger())
        m_qmlDebugServices = QmlDebug::QmlDebuggerServices;
    else if (runMode == ProjectExplorer::Constants::QML_PROFILER_RUN_MODE)
        m_qmlDebugServices = QmlDebug::QmlProfilerServices;
    else
        m_qmlDebugServices = QmlDebug::NoQmlDebugServices;
    QString channel = runConfig->remoteChannel();
    QTC_CHECK(channel.startsWith(QLatin1Char(':')));
    m_localGdbServerPort = Utils::Port(channel.mid(1).toUShort());
    QTC_CHECK(m_localGdbServerPort.isValid());
    if (m_qmlDebugServices != QmlDebug::NoQmlDebugServices) {
        QTcpServer server;
        QTC_ASSERT(server.listen(QHostAddress::LocalHost)
                   || server.listen(QHostAddress::LocalHostIPv6),
                   qDebug() << tr("No free ports available on host for QML debugging."));
        m_qmlPort = Utils::Port(server.serverPort());
    } else {
        m_qmlPort = Utils::Port();
    }
    m_adb = AndroidConfigurations::currentConfig().adbToolPath().toString();

    QString packageDir = "/data/data/" + m_packageName;
    m_pingFile = packageDir + "/debug-ping";
    m_pongFile = "/data/local/tmp/qt/debug-pong-" + m_packageName;
    m_gdbserverSocket = packageDir + "/debug-socket";
    const QtSupport::BaseQtVersion *version = QtSupport::QtKitInformation::qtVersion(
                runConfig->target()->kit());
    if (version && version->qtVersion() >=  QtSupport::QtVersionNumber(5, 4, 0))
        m_gdbserverPath = packageDir + "/lib/libgdbserver.so";
    else
        m_gdbserverPath = packageDir + "/lib/gdbserver";

    if (version && version->qtVersion() >= QtSupport::QtVersionNumber(5, 4, 0)) {
        if (qEnvironmentVariableIsSet("QTC_ANDROID_USE_FILE_HANDSHAKE"))
            m_handShakeMethod = PingPongFiles;
    } else {
        m_handShakeMethod = PingPongFiles;
    }

    if (qEnvironmentVariableIsSet("QTC_ANDROID_SOCKET_HANDSHAKE_PORT")) {
        QByteArray envData = qgetenv("QTC_ANDROID_SOCKET_HANDSHAKE_PORT");
        if (!envData.isEmpty()) {
            bool ok = false;
            int port = 0;
            port = envData.toInt(&ok);
            if (ok && port > 0 && port < 65535) {
                m_socketHandShakePort = port;
                m_customPort = true;
            }
        }
    }
}

AndroidRunnerWorker::~AndroidRunnerWorker()
{
}

void AndroidRunnerWorker::forceStop()
{
    runAdb(selector() << "shell" << "am" << "force-stop" << m_packageName, nullptr, 30);

    for (auto it = m_processPids.constBegin(); it != m_processPids.constEnd(); ++it) {
        emit pidLost(*it);
        adbKill(*it);
    }
    m_processPids.clear();
    m_pidsBuffer.clear();
}

void AndroidRunnerWorker::asyncStart(const QString &intentName,
                                     const QVector<QStringList> &adbCommands)
{
    forceStop();

    // Start the logcat process before app starts.
    std::unique_ptr<QProcess, decltype(&deleter)> logcatProcess(new QProcess, deleter);
    connect(logcatProcess.get(), &QProcess::readyReadStandardOutput,
            this, &AndroidRunnerWorker::logcatReadStandardOutput);
    connect(logcatProcess.get(), &QProcess::readyReadStandardError,
            this, &AndroidRunnerWorker::logcatReadStandardError);

    // Its assumed that the device or avd returned by selector() is online.
    QStringList logcatArgs = selector() << "logcat" << "-v" << "time";
    if (deviceSdkVersion() > 20)
        logcatArgs << "-T" << "0";
    logcatProcess->start(m_adb, logcatArgs);

    QString errorMessage;

    if (m_useCppDebugger)
        runAdb(selector() << "shell" << "rm" << m_pongFile); // Remove pong file.

    foreach (const QStringList &entry, adbCommands)
        runAdb(selector() << entry);

    QStringList args = selector();
    args << "shell" << "am" << "start" << "-n" << intentName;

    if (m_useCppDebugger) {
        if (!runAdb(selector() << "forward"
                    << QString::fromLatin1("tcp:%1").arg(m_localGdbServerPort.number())
                    << "localfilesystem:" + m_gdbserverSocket, &errorMessage)) {
            emit remoteProcessFinished(tr("Failed to forward C++ debugging ports. Reason: %1.").arg(errorMessage));
            return;
        }

        const QString pingPongSocket(m_packageName + ".ping_pong_socket");
        args << "-e" << "debug_ping" << "true";
        if (m_handShakeMethod == SocketHandShake) {
            args << "-e" << "ping_socket" << pingPongSocket;
        } else if (m_handShakeMethod == PingPongFiles) {
            args << "-e" << "ping_file" << m_pingFile;
            args << "-e" << "pong_file" << m_pongFile;
        }

        QString gdbserverCommand = QString::fromLatin1(adbShellAmNeedsQuotes() ? "\"%1 --multi +%2\"" : "%1 --multi +%2")
                .arg(m_gdbserverPath).arg(m_gdbserverSocket);
        args << "-e" << "gdbserver_command" << gdbserverCommand;
        args << "-e" << "gdbserver_socket" << m_gdbserverSocket;

        if (m_handShakeMethod == SocketHandShake) {
            const QString port = QString::fromLatin1("tcp:%1").arg(m_socketHandShakePort);
            if (!runAdb(selector() << "forward" << port << ("localabstract:" + pingPongSocket),
                        &errorMessage)) {
                emit remoteProcessFinished(tr("Failed to forward ping pong ports. Reason: %1.")
                                           .arg(errorMessage));
                return;
            }
        }
    }

    if (m_qmlDebugServices != QmlDebug::NoQmlDebugServices) {
        // currently forward to same port on device and host
        const QString port = QString::fromLatin1("tcp:%1").arg(m_qmlPort.number());
        if (!runAdb(selector() << "forward" << port << port, &errorMessage)) {
            emit remoteProcessFinished(tr("Failed to forward QML debugging ports. Reason: %1.")
                                       .arg(errorMessage));
            return;
        }

        args << "-e" << "qml_debug" << "true"
             << "-e" << "qmljsdebugger"
             << QString::fromLatin1("port:%1,block,services:%2")
                .arg(m_qmlPort.number()).arg(QmlDebug::qmlDebugServices(m_qmlDebugServices));
    }

    if (!runAdb(args, &errorMessage)) {
        emit remoteProcessFinished(tr("Failed to start the activity. Reason: %1.")
                                   .arg(errorMessage));
        return;
    }

    if (m_useCppDebugger) {
        if (m_handShakeMethod == SocketHandShake) {
            //Handling socket
            bool wasSuccess = false;
            const int maxAttempts = 20; //20 seconds
            m_socket.reset(new QTcpSocket());
            for (int i = 0; i < maxAttempts; i++) {

                QThread::sleep(1); // give Android time to start process
                m_socket->connectToHost(QHostAddress(QStringLiteral("127.0.0.1")),
                                        m_socketHandShakePort);
                if (!m_socket->waitForConnected())
                    continue;

                if (!m_socket->waitForReadyRead()) {
                    m_socket->close();
                    continue;
                }

                const QByteArray pid = m_socket->readLine();
                if (pid.isEmpty()) {
                    m_socket->close();
                    continue;
                }

                wasSuccess = true;

                break;
            }

            if (!m_customPort) {
                // increment running port to avoid clash when using multiple
                // debug sessions at the same time
                m_socketHandShakePort++;
                // wrap ports around to avoid overflow
                if (m_socketHandShakePort == MAX_SOCKET_HANDSHAKE_PORT)
                    m_socketHandShakePort = MIN_SOCKET_HANDSHAKE_PORT;
            }

            if (!wasSuccess) {
                emit remoteProcessFinished(tr("Failed to contact debugging port."));
                return;
            }
        } else {
            // Handling ping.
            for (int i = 0; ; ++i) {
                Utils::TemporaryFile tmp("pingpong");
                tmp.open();
                tmp.close();

                runAdb(selector() << "pull" << m_pingFile << tmp.fileName());

                QFile res(tmp.fileName());
                const bool doBreak = res.size();
                res.remove();
                if (doBreak)
                    break;

                if (i == 20) {
                    emit remoteProcessFinished(tr("Unable to start \"%1\".").arg(m_packageName));
                    return;
                }
                qDebug() << "WAITING FOR " << tmp.fileName();
                QThread::msleep(500);
            }
        }

    }

    QTC_ASSERT(!m_adbLogcatProcess, /**/);
    m_adbLogcatProcess = std::move(logcatProcess);

    m_timeoutTimer.reset(new QTimer);
    m_timeoutTimer->setSingleShot(true);
    connect(m_timeoutTimer.data(), &QTimer::timeout,
            this,[this] { onProcessIdChanged(PidInfo{}); });
    m_timeoutTimer->start(APP_START_TIMEOUT);

    m_pidsFinderProcess.reset(new QProcess);
    m_pidsFinderProcess->setProcessChannelMode(QProcess::MergedChannels);
    connect(m_pidsFinderProcess.get(), &QProcess::readyRead, this, &AndroidRunnerWorker::findProcessPids);
    connect(m_pidsFinderProcess.get(),
            static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
            this, [this] { onProcessIdChanged(PidInfo{}); });
    m_pidsFinderProcess->start(m_adb, selector() << "shell" << pidScript);
}

bool AndroidRunnerWorker::adbShellAmNeedsQuotes()
{
    // Between Android SDK Tools version 24.3.1 and 24.3.4 the quoting
    // needs for the 'adb shell am start ...' parameters changed.
    // Run a test to find out on what side of the fence we live.
    // The command will fail with a complaint about the "--dummy"
    // option on newer SDKs, and with "No intent supplied" on older ones.
    // In case the test itself fails assume a new SDK.
    Utils::SynchronousProcess adb;
    adb.setTimeoutS(10);
    Utils::SynchronousProcessResponse response
            = adb.run(m_adb, selector() << "shell" << "am" << "start"
                                        << "-e" << "dummy" << "dummy --dummy");
    if (response.result == Utils::SynchronousProcessResponse::StartFailed
            || response.result != Utils::SynchronousProcessResponse::Finished)
        return true;

    const QString output = response.allOutput();
    const bool oldSdk = output.contains("Error: No intent supplied");
    return !oldSdk;
}

bool AndroidRunnerWorker::runAdb(const QStringList &args, QString *exitMessage, int timeoutS)
{
    Utils::SynchronousProcess adb;
    adb.setTimeoutS(timeoutS);
    Utils::SynchronousProcessResponse response
            = adb.run(m_adb, args);
    if (exitMessage)
        *exitMessage = response.exitMessage(m_adb, timeoutS);
    return response.result == Utils::SynchronousProcessResponse::Finished;
}

int AndroidRunnerWorker::deviceSdkVersion()
{
    Utils::SynchronousProcess adb;
    adb.setTimeoutS(10);
    Utils::SynchronousProcessResponse response
            = adb.run(m_adb, selector() << "shell" << "getprop" << "ro.build.version.sdk");
    if (response.result == Utils::SynchronousProcessResponse::StartFailed
            || response.result != Utils::SynchronousProcessResponse::Finished)
        return -1;

    return response.allOutput().trimmed().toInt();
}

void AndroidRunnerWorker::handleRemoteDebuggerRunning()
{
    if (m_useCppDebugger) {
        if (m_handShakeMethod == SocketHandShake) {
            m_socket->write("OK");
            m_socket->waitForBytesWritten();
            m_socket->close();
        } else {
            Utils::TemporaryFile tmp("pingpong");
            tmp.open();

            runAdb(selector() << "push" << tmp.fileName() << m_pongFile);
        }
        QTC_CHECK(!m_processPids.isEmpty());
    }
    emit remoteProcessStarted(m_localGdbServerPort, m_qmlPort);
}

void AndroidRunnerWorker::findProcessPids()
{
    static QMap<qint64, QByteArray> extractedPids;
    static auto oldPids = m_processPids;

    m_pidsBuffer += m_pidsFinderProcess->readAll();
    while (!m_pidsBuffer.isEmpty()) {
        const int to = m_pidsBuffer.indexOf('\n');
        if (to < 0)
            break;

        if (to == 0) {
            m_pidsBuffer = m_pidsBuffer.mid(1);
            continue;
        }

        // = is used to delimit ps outputs
        // is needed to know when an existins PID is killed
        if (m_pidsBuffer[0] != '=') {
            QByteArray tuple = m_pidsBuffer.left(to + 1).simplified();
            QList<QByteArray> parts = tuple.split(':');
            QByteArray commandName = parts.takeFirst();
            if (QString::fromLocal8Bit(commandName) == m_packageName) {
                auto pid = parts.last().toLongLong();
                if (!m_processPids.contains(pid)) {
                    extractedPids[pid] = commandName + (parts.length() == 2
                                                        ? ":" + parts.first() : QByteArray{});
                 } else {
                    oldPids.remove(pid);
                }
            }
        } else {
            // Add new PIDs
            for (auto it = extractedPids.constBegin(); it != extractedPids.constEnd(); ++it) {
                onProcessIdChanged(PidInfo(it.key(), PidStatus::Found,
                                           QString::fromLocal8Bit(it.value())));
            }
            extractedPids.clear();

            // Remove the dead ones
            for (auto it = oldPids.constBegin(); it != oldPids.constEnd(); ++it)
                onProcessIdChanged(PidInfo(*it, PidStatus::Lost));

            // Save the current non dead PIDs
            oldPids = m_processPids;
            if (m_processPids.isEmpty()) {
                extractedPids.clear();
                m_pidsBuffer.clear();
                break;
            }
        }
        m_pidsBuffer = m_pidsBuffer.mid(to + 1);
    }
}

void AndroidRunnerWorker::asyncStop(const QVector<QStringList> &adbCommands)
{
    m_timeoutTimer.reset();
    m_pidsFinderProcess.reset();
    if (!m_processPids.isEmpty())
        forceStop();

    foreach (const QStringList &entry, adbCommands)
        runAdb(selector() << entry);

    m_adbLogcatProcess.reset();
    emit remoteProcessFinished(QLatin1String("\n\n") +
                               tr("\"%1\" terminated.").arg(m_packageName));
}

void AndroidRunnerWorker::setAdbParameters(const QString &packageName, const QStringList &selector)
{
    m_packageName = packageName;
    m_selector = selector;
}

void AndroidRunnerWorker::logcatProcess(const QByteArray &text, QByteArray &buffer, bool onlyError)
{
    QList<QByteArray> lines = text.split('\n');
    // lines always contains at least one item
    lines[0].prepend(buffer);
    if (!lines.last().endsWith('\n')) {
        // incomplete line
        buffer = lines.last();
        lines.removeLast();
    } else {
        buffer.clear();
    }

    foreach (const QByteArray &msg, lines) {
        const QString line = QString::fromUtf8(msg.trimmed());
        if (onlyError)
            emit remoteErrorOutput(line);
        else
            emit remoteOutput(line);
    }
}

void AndroidRunnerWorker::onProcessIdChanged(PidInfo pidInfo)
{
    // Don't write to m_psProc from a different thread
    QTC_ASSERT(QThread::currentThread() == thread(), return);

    auto isFirst = m_processPids.isEmpty();
    if (pidInfo.status == PidStatus::Lost) {
        m_processPids.remove(pidInfo.pid);
        emit pidLost(pidInfo.pid);
    } else {
        m_processPids.insert(pidInfo.pid);
        emit pidFound(pidInfo.pid, pidInfo.name);
    }

    if (m_processPids.isEmpty() || pidInfo.pid == -1) {
        emit remoteProcessFinished(QLatin1String("\n\n") + tr("\"%1\" died.")
                                   .arg(m_packageName));
        // App died/killed. Reset log and monitor processes.
        forceStop();
        m_adbLogcatProcess.reset();
        m_timeoutTimer.reset();
    } else if (isFirst) {
        m_timeoutTimer.reset();
        if (m_useCppDebugger) {
            // This will be funneled to the engine to actually start and attach
            // gdb. Afterwards this ends up in handleRemoteDebuggerRunning() below.
            QByteArray serverChannel = ':' + QByteArray::number(m_localGdbServerPort.number());
            emit remoteServerRunning(serverChannel, pidInfo.pid);
        } else if (m_qmlDebugServices == QmlDebug::QmlDebuggerServices) {
            // This will be funneled to the engine to actually start and attach
            // gdb. Afterwards this ends up in handleRemoteDebuggerRunning() below.
            QByteArray serverChannel = QByteArray::number(m_qmlPort.number());
            emit remoteServerRunning(serverChannel, pidInfo.pid);
        } else if (m_qmlDebugServices == QmlDebug::QmlProfilerServices) {
            emit remoteProcessStarted(Utils::Port(), m_qmlPort);
        } else {
            // Start without debugging.
            emit remoteProcessStarted(Utils::Port(), Utils::Port());
        }
        logcatReadStandardOutput();
    }
}

void AndroidRunnerWorker::logcatReadStandardError()
{
    if (!m_processPids.isEmpty() && m_adbLogcatProcess)
        logcatProcess(m_adbLogcatProcess->readAllStandardError(), m_stderrBuffer, true);
}

void AndroidRunnerWorker::logcatReadStandardOutput()
{
    if (!m_processPids.isEmpty() && m_adbLogcatProcess)
        logcatProcess(m_adbLogcatProcess->readAllStandardOutput(), m_stdoutBuffer, false);
}

void AndroidRunnerWorker::adbKill(qint64 pid)
{
    runAdb(selector() << "shell" << "kill" << "-9" << QString::number(pid));
    runAdb(selector() << "shell" << "run-as" << m_packageName
                      << "kill" << "-9" << QString::number(pid));
}

AndroidRunner::AndroidRunner(QObject *parent, RunConfiguration *runConfig, Core::Id runMode)
    : QObject(parent), m_runConfig(qobject_cast<AndroidRunConfiguration *>(runConfig))
{
    static const int metaTypes[] = {
        qRegisterMetaType<QVector<QStringList> >("QVector<QStringList>"),
        qRegisterMetaType<Utils::Port>("Utils::Port")
    };
    Q_UNUSED(metaTypes);

    m_checkAVDTimer.setInterval(2000);
    connect(&m_checkAVDTimer, &QTimer::timeout, this, &AndroidRunner::checkAVD);

    Target *target = runConfig->target();
    m_androidRunnable.intentName = AndroidManager::intentName(target);
    m_androidRunnable.packageName = m_androidRunnable.intentName.left(
                m_androidRunnable.intentName.indexOf(QLatin1Char('/')));
    m_androidRunnable.deviceSerialNumber = AndroidManager::deviceSerialNumber(target);

    m_worker.reset(new AndroidRunnerWorker(
                m_runConfig, runMode, m_androidRunnable.packageName,
                AndroidDeviceInfo::adbSelector(m_androidRunnable.deviceSerialNumber)));
    m_worker->moveToThread(&m_thread);

    connect(this, &AndroidRunner::asyncStart, m_worker.data(), &AndroidRunnerWorker::asyncStart);
    connect(this, &AndroidRunner::asyncStop, m_worker.data(), &AndroidRunnerWorker::asyncStop);
    connect(this, &AndroidRunner::adbParametersChanged,
            m_worker.data(), &AndroidRunnerWorker::setAdbParameters);
    connect(this, &AndroidRunner::remoteDebuggerRunning, m_worker.data(),
            &AndroidRunnerWorker::handleRemoteDebuggerRunning);

    connect(m_worker.data(), &AndroidRunnerWorker::remoteServerRunning,
            this, &AndroidRunner::remoteServerRunning);
    connect(m_worker.data(), &AndroidRunnerWorker::remoteProcessStarted,
            this, &AndroidRunner::remoteProcessStarted);
    connect(m_worker.data(), &AndroidRunnerWorker::remoteProcessFinished,
            this, &AndroidRunner::remoteProcessFinished);
    connect(m_worker.data(), &AndroidRunnerWorker::remoteOutput,
            this, &AndroidRunner::remoteOutput);
    connect(m_worker.data(), &AndroidRunnerWorker::remoteErrorOutput,
            this, &AndroidRunner::remoteErrorOutput);
    connect(m_worker.data(), &AndroidRunnerWorker::pidFound,
            this, &AndroidRunner::pidFound);
    connect(m_worker.data(), &AndroidRunnerWorker::pidLost,
            this, &AndroidRunner::pidLost);

    m_thread.start();
}

AndroidRunner::~AndroidRunner()
{
    m_thread.quit();
    m_thread.wait();
}

void AndroidRunner::start()
{
    if (!ProjectExplorerPlugin::projectExplorerSettings().deployBeforeRun) {
        // User choose to run the app without deployment. Start the AVD if not running.
       launchAVD();
       if (!m_launchedAVDName.isEmpty()) {
           m_checkAVDTimer.start();
           return;
       }
    }

    emit asyncStart(m_androidRunnable.intentName, m_androidRunnable.beforeStartADBCommands);
}

void AndroidRunner::stop()
{
    if (m_checkAVDTimer.isActive()) {
        m_checkAVDTimer.stop();
        emit remoteProcessFinished(QLatin1String("\n\n") + tr("\"%1\" terminated.")
                                   .arg(m_androidRunnable.packageName));
        return;
    }

    emit asyncStop(m_androidRunnable.afterFinishADBCommands);
}

QString AndroidRunner::displayName() const
{
    return m_androidRunnable.packageName;
}

void AndroidRunner::setRunnable(const AndroidRunnable &runnable)
{
    if (runnable != m_androidRunnable) {
        m_androidRunnable = runnable;
        emit adbParametersChanged(runnable.packageName,
                                  AndroidDeviceInfo::adbSelector(runnable.deviceSerialNumber));
    }
}

void AndroidRunner::launchAVD()
{
    if (!m_runConfig->target() && !m_runConfig->target()->project())
        return;

    int deviceAPILevel = AndroidManager::minimumSDK(m_runConfig->target());
    QString targetArch = AndroidManager::targetArch(m_runConfig->target());

    // Get AVD info.
    AndroidDeviceInfo info = AndroidConfigurations::showDeviceDialog(
                m_runConfig->target()->project(), deviceAPILevel, targetArch,
                AndroidConfigurations::None);
    AndroidManager::setDeviceSerialNumber(m_runConfig->target(), info.serialNumber);
    m_androidRunnable.deviceSerialNumber = info.serialNumber;
    emit adbParametersChanged(m_androidRunnable.packageName,
                              AndroidDeviceInfo::adbSelector(info.serialNumber));
    if (info.isValid()) {
        AndroidAvdManager avdManager;
        if (avdManager.findAvd(info.avdname).isEmpty()) {
            bool launched = avdManager.startAvdAsync(info.avdname);
            m_launchedAVDName = launched ? info.avdname:"";
        } else {
            m_launchedAVDName.clear();
        }
    }
}

void AndroidRunner::checkAVD()
{
    const AndroidConfig &config = AndroidConfigurations::currentConfig();
    AndroidAvdManager avdManager(config);
    QString serialNumber = avdManager.findAvd(m_launchedAVDName);
    if (!serialNumber.isEmpty())
        return; // try again on next timer hit

    if (avdManager.isAvdBooted(serialNumber)) {
        m_checkAVDTimer.stop();
        AndroidManager::setDeviceSerialNumber(m_runConfig->target(), serialNumber);
        emit asyncStart(m_androidRunnable.intentName, m_androidRunnable.beforeStartADBCommands);
    } else if (!config.isConnected(serialNumber)) {
        // device was disconnected
        m_checkAVDTimer.stop();
    }
}

} // namespace Internal
} // namespace Android

#include "androidrunner.moc"
