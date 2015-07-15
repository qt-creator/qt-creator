/**************************************************************************
**
** Copyright (C) 2015 BogDan Vatra <bog_dan_ro@yahoo.com>
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "androidrunner.h"

#include "androiddeployqtstep.h"
#include "androidconfigurations.h"
#include "androidglobal.h"
#include "androidrunconfiguration.h"
#include "androidmanager.h"

#include <debugger/debuggerrunconfigurationaspect.h>
#include <projectexplorer/target.h>
#include <qtsupport/qtkitinformation.h>
#include <utils/qtcassert.h>

#include <QApplication>
#include <QDir>
#include <QTime>
#include <QtConcurrentRun>
#include <QTemporaryFile>
#include <QTcpServer>
#include <QTcpSocket>

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

typedef QLatin1String _;
const int MIN_SOCKET_HANDSHAKE_PORT = 20001;
const int MAX_SOCKET_HANDSHAKE_PORT = 20999;

static int socketHandShakePort = MIN_SOCKET_HANDSHAKE_PORT;

AndroidRunner::AndroidRunner(QObject *parent,
                             AndroidRunConfiguration *runConfig,
                             Core::Id runMode)
    : QThread(parent), m_handShakeMethod(SocketHandShake), m_socket(0),
      m_customPort(false)
{
    m_tries = 0;
    Debugger::DebuggerRunConfigurationAspect *aspect
            = runConfig->extraAspect<Debugger::DebuggerRunConfigurationAspect>();
    const bool debuggingMode = (runMode == ProjectExplorer::Constants::DEBUG_RUN_MODE || runMode == ProjectExplorer::Constants::DEBUG_RUN_MODE_WITH_BREAK_ON_MAIN);
    m_useCppDebugger = debuggingMode && aspect->useCppDebugger();
    m_useQmlDebugger = debuggingMode && aspect->useQmlDebugger();
    QString channel = runConfig->remoteChannel();
    QTC_CHECK(channel.startsWith(QLatin1Char(':')));
    m_localGdbServerPort = channel.mid(1).toUShort();
    QTC_CHECK(m_localGdbServerPort);
    m_useQmlProfiler = runMode == ProjectExplorer::Constants::QML_PROFILER_RUN_MODE;
    if (m_useQmlDebugger || m_useQmlProfiler) {
        QTcpServer server;
        QTC_ASSERT(server.listen(QHostAddress::LocalHost)
                   || server.listen(QHostAddress::LocalHostIPv6),
                   qDebug() << tr("No free ports available on host for QML debugging."));
        m_qmlPort = server.serverPort();
    } else {
        m_qmlPort = -1;
    }
    ProjectExplorer::Target *target = runConfig->target();
    m_useLocalQtLibs = AndroidManager::useLocalLibs(target);
    if (m_useLocalQtLibs) {
        int deviceApiLevel = AndroidManager::minimumSDK(target);
        m_localLibs = AndroidManager::loadLocalLibs(target, deviceApiLevel);
        m_localJars = AndroidManager::loadLocalJars(target, deviceApiLevel);
        m_localJarsInitClasses = AndroidManager::loadLocalJarsInitClasses(target, deviceApiLevel);
    }
    m_intentName = AndroidManager::intentName(target);
    m_packageName = m_intentName.left(m_intentName.indexOf(QLatin1Char('/')));

    m_deviceSerialNumber = AndroidManager::deviceSerialNumber(target);
    m_processPID = -1;
    m_adb = AndroidConfigurations::currentConfig().adbToolPath().toString();
    m_selector = AndroidDeviceInfo::adbSelector(m_deviceSerialNumber);

    QString packageDir = _("/data/data/") + m_packageName;
    m_pingFile = packageDir + _("/debug-ping");
    m_pongFile = _("/data/local/tmp/qt/debug-pong-") + m_packageName;
    m_gdbserverSocket = packageDir + _("/debug-socket");
    const QtSupport::BaseQtVersion *version = QtSupport::QtKitInformation::qtVersion(target->kit());
    if (version && version->qtVersion() >=  QtSupport::QtVersionNumber(5, 4, 0))
        m_gdbserverPath = packageDir + _("/lib/libgdbserver.so");
    else
        m_gdbserverPath = packageDir + _("/lib/gdbserver");


    m_gdbserverCommand = m_gdbserverPath + _(" --multi +") + m_gdbserverSocket;
    // Detect busybox, as we need to pass -w to ps to get wide output.
    QProcess psProc;
    psProc.start(m_adb, selector() << _("shell") << _("readlink") << _("$(which ps)"));
    psProc.waitForFinished();
    QByteArray which = psProc.readAll();
    m_isBusyBox = which.startsWith("busybox");

    m_checkPIDTimer.setInterval(1000);

    connect(&m_adbLogcatProcess, SIGNAL(readyReadStandardOutput()), SLOT(logcatReadStandardOutput()));
    connect(&m_adbLogcatProcess, SIGNAL(readyReadStandardError()), SLOT(logcatReadStandardError()));
    connect(&m_checkPIDTimer, SIGNAL(timeout()), SLOT(checkPID()));

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
                socketHandShakePort = port;
                m_customPort = true;
            }
        }
    }

    m_logCatRegExp = QRegExp(QLatin1String("[0-9\\-]*"  // date
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
                                          ));
}

AndroidRunner::~AndroidRunner()
{
    //stop();
    delete m_socket;
}

static int extractPidFromChunk(const QByteArray &chunk, int from)
{
    int pos1 = chunk.indexOf(' ', from);
    if (pos1 == -1)
        return -1;
    while (chunk[pos1] == ' ')
        ++pos1;
    int pos3 = chunk.indexOf(' ', pos1);
    int pid = chunk.mid(pos1, pos3 - pos1).toInt();
    return pid;
}

static int extractPid(const QString &exeName, const QByteArray &psOutput)
{
    const QByteArray needle = exeName.toUtf8() + '\r';
    const int to = psOutput.indexOf(needle);
    if (to == -1)
        return -1;
    const int from = psOutput.lastIndexOf('\n', to);
    if (from == -1)
        return -1;
    return extractPidFromChunk(psOutput, from);
}

QByteArray AndroidRunner::runPs()
{
    QProcess psProc;
    QStringList args = m_selector;
    args << _("shell") << _("ps");
    if (m_isBusyBox)
        args << _("-w");

    psProc.start(m_adb, args);
    psProc.waitForFinished();
    return psProc.readAll();
}

void AndroidRunner::checkPID()
{
    QByteArray psOut = runPs();
    m_processPID = extractPid(m_packageName, psOut);

    if (m_processPID == -1) {
        if (m_wasStarted) {
            m_wasStarted = false;
            m_checkPIDTimer.stop();
            emit remoteProcessFinished(QLatin1String("\n\n") + tr("\"%1\" died.").arg(m_packageName));
        } else {
            if (++m_tries > 3)
                emit remoteProcessFinished(QLatin1String("\n\n") + tr("Unable to start \"%1\".").arg(m_packageName));
        }
    } else if (!m_wasStarted){
        if (m_useCppDebugger) {
            // This will be funneled to the engine to actually start and attach
            // gdb. Afterwards this ends up in handleRemoteDebuggerRunning() below.
            QByteArray serverChannel = ':' + QByteArray::number(m_localGdbServerPort);
            emit remoteServerRunning(serverChannel, m_processPID);
        } else if (m_useQmlDebugger) {
            // This will be funneled to the engine to actually start and attach
            // gdb. Afterwards this ends up in handleRemoteDebuggerRunning() below.
            QByteArray serverChannel = QByteArray::number(m_qmlPort);
            emit remoteServerRunning(serverChannel, m_processPID);
        } else if (m_useQmlProfiler) {
            emit remoteProcessStarted(-1, m_qmlPort);
        } else {
            // Start without debugging.
            emit remoteProcessStarted(-1, -1);
        }
        m_wasStarted = true;
        logcatReadStandardOutput();
    }
}

void AndroidRunner::forceStop()
{
    QProcess proc;
    proc.start(m_adb, selector() << _("shell") << _("am") << _("force-stop")
               << m_packageName);
    proc.waitForFinished();

    // try killing it via kill -9
    const QByteArray out = runPs();
    int from = 0;
    while (1) {
        const int to = out.indexOf('\n', from);
        if (to == -1)
            break;
        QString line = QString::fromUtf8(out.data() + from, to - from - 1);
        if (line.endsWith(m_packageName) || line.endsWith(m_gdbserverPath)) {
            int pid = extractPidFromChunk(out, from);
            adbKill(pid);
        }
        from = to + 1;
    }
}

void AndroidRunner::start()
{
    m_adbLogcatProcess.start(m_adb, selector() << _("logcat"));
    QtConcurrent::run(this, &AndroidRunner::asyncStart);
}

void AndroidRunner::asyncStart()
{
    QMutexLocker locker(&m_mutex);
    forceStop();

    if (m_useCppDebugger) {
        // Remove pong file.
        QProcess adb;
        adb.start(m_adb, selector() << _("shell") << _("rm") << m_pongFile);
        adb.waitForFinished();
    }

    QStringList args = selector();
    args << _("shell") << _("am") << _("start") << _("-n") << m_intentName;

    if (m_useCppDebugger) {
        QProcess adb;
        adb.start(m_adb, selector() << _("forward")
                  << QString::fromLatin1("tcp:%1").arg(m_localGdbServerPort)
                  << _("localfilesystem:") + m_gdbserverSocket);
        if (!adb.waitForStarted()) {
            emit remoteProcessFinished(tr("Failed to forward C++ debugging ports. Reason: %1.").arg(adb.errorString()));
            return;
        }
        if (!adb.waitForFinished(10000)) {
            emit remoteProcessFinished(tr("Failed to forward C++ debugging ports."));
            return;
        }

        const QString pingPongSocket(m_packageName + _(".ping_pong_socket"));
        args << _("-e") << _("debug_ping") << _("true");
        if (m_handShakeMethod == SocketHandShake) {
            args << _("-e") << _("ping_socket") << pingPongSocket;
        } else if (m_handShakeMethod == PingPongFiles) {
            args << _("-e") << _("ping_file") << m_pingFile;
            args << _("-e") << _("pong_file") << m_pongFile;
        }
        args << _("-e") << _("gdbserver_command") << m_gdbserverCommand;
        args << _("-e") << _("gdbserver_socket") << m_gdbserverSocket;

        if (m_handShakeMethod == SocketHandShake) {
            QProcess adb;
            const QString port = QString::fromLatin1("tcp:%1").arg(socketHandShakePort);
            adb.start(m_adb, selector() << _("forward") << port << _("localabstract:") + pingPongSocket);
            if (!adb.waitForStarted()) {
                emit remoteProcessFinished(tr("Failed to forward ping pong ports. Reason: %1.").arg(adb.errorString()));
                return;
            }
            if (!adb.waitForFinished()) {
                emit remoteProcessFinished(tr("Failed to forward ping pong ports."));
                return;
            }
        }
    }

    if (m_useQmlDebugger || m_useQmlProfiler) {
        // currently forward to same port on device and host
        const QString port = QString::fromLatin1("tcp:%1").arg(m_qmlPort);
        QProcess adb;
        adb.start(m_adb, selector() << _("forward") << port << port);
        if (!adb.waitForStarted()) {
            emit remoteProcessFinished(tr("Failed to forward QML debugging ports. Reason: %1.").arg(adb.errorString()));
            return;
        }
        if (!adb.waitForFinished()) {
            emit remoteProcessFinished(tr("Failed to forward QML debugging ports."));
            return;
        }
        args << _("-e") << _("qml_debug") << _("true");
        args << _("-e") << _("qmljsdebugger") << QString::fromLatin1("port:%1,block").arg(m_qmlPort);
    }

    QProcess adb;
    adb.start(m_adb, args);
    if (!adb.waitForStarted()) {
        emit remoteProcessFinished(tr("Failed to start the activity. Reason: %1.").arg(adb.errorString()));
        return;
    }
    if (!adb.waitForFinished(10000)) {
        adb.terminate();
        emit remoteProcessFinished(tr("Unable to start \"%1\".").arg(m_packageName));
        return;
    }

    if (m_useCppDebugger) {
        if (m_handShakeMethod == SocketHandShake) {
            //Handling socket
            bool wasSuccess = false;
            const int maxAttempts = 20; //20 seconds
            if (m_socket)
                delete m_socket;
            m_socket = new QTcpSocket();
            for (int i = 0; i < maxAttempts; i++) {

                QThread::sleep(1); // give Android time to start process
                m_socket->connectToHost(QHostAddress(QStringLiteral("127.0.0.1")), socketHandShakePort);
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
                m_socket->moveToThread(QApplication::instance()->thread());

                break;
            }

            if (!wasSuccess)
                emit remoteProcessFinished(tr("Failed to contact debugging port."));

            if (!m_customPort) {
                // increment running port to avoid clash when using multiple
                // debug sessions at the same time
                socketHandShakePort++;
                // wrap ports around to avoid overflow
                if (socketHandShakePort == MAX_SOCKET_HANDSHAKE_PORT)
                    socketHandShakePort = MIN_SOCKET_HANDSHAKE_PORT;
            }
        } else {
            // Handling ping.
            for (int i = 0; ; ++i) {
                QTemporaryFile tmp(QDir::tempPath() + _("/pingpong"));
                tmp.open();
                tmp.close();

                QProcess process;
                process.start(m_adb, selector() << _("pull") << m_pingFile << tmp.fileName());
                process.waitForFinished();

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

    m_tries = 0;
    m_wasStarted = false;
    QMetaObject::invokeMethod(&m_checkPIDTimer, "start");
}

void AndroidRunner::handleRemoteDebuggerRunning()
{
    if (m_useCppDebugger) {
        if (m_handShakeMethod == SocketHandShake) {
            m_socket->write("OK");
            m_socket->waitForBytesWritten();
            m_socket->close();
        } else {
            QTemporaryFile tmp(QDir::tempPath() + _("/pingpong"));
            tmp.open();

            QProcess process;
            process.start(m_adb, selector() << _("push") << tmp.fileName() << m_pongFile);
            process.waitForFinished();
        }
        QTC_CHECK(m_processPID != -1);
    }
    emit remoteProcessStarted(m_localGdbServerPort, m_qmlPort);
}

void AndroidRunner::stop()
{
    QMutexLocker locker(&m_mutex);
    m_checkPIDTimer.stop();
    m_tries = 0;
    if (m_processPID != -1) {
        forceStop();
        emit remoteProcessFinished(QLatin1String("\n\n") + tr("\"%1\" terminated.").arg(m_packageName));
    }
    //QObject::disconnect(&m_adbLogcatProcess, 0, this, 0);
    m_adbLogcatProcess.kill();
    m_adbLogcatProcess.waitForFinished();
}

void AndroidRunner::logcatProcess(const QByteArray &text, QByteArray &buffer, bool onlyError)
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

    QString pidString = QString::number(m_processPID);
    foreach (const QByteArray &msg, lines) {
        const QString line = QString::fromUtf8(msg).trimmed() + QLatin1Char('\n');
        if (!line.contains(pidString))
            continue;
        if (m_logCatRegExp.exactMatch(line)) {
            // Android M
            if (m_logCatRegExp.cap(1) == pidString) {
                const QString &messagetype = m_logCatRegExp.cap(2);
                QString output = line.mid(m_logCatRegExp.pos(2));

                if (onlyError
                        || messagetype == QLatin1String("F")
                        || messagetype == QLatin1String("E")
                        || messagetype == QLatin1String("W"))
                    emit remoteErrorOutput(output);
                else
                    emit remoteOutput(output);
            }
        } else {
            if (onlyError || line.startsWith(_("F/"))
                    || line.startsWith(_("E/"))
                    || line.startsWith(_("W/")))
                emit remoteErrorOutput(line);
            else
                emit remoteOutput(line);
        }
    }
}

void AndroidRunner::logcatReadStandardError()
{
    if (m_processPID != -1)
        logcatProcess(m_adbLogcatProcess.readAllStandardError(), m_stderrBuffer, true);
}

void AndroidRunner::logcatReadStandardOutput()
{
    if (m_processPID != -1)
        logcatProcess(m_adbLogcatProcess.readAllStandardOutput(), m_stdoutBuffer, false);
}

void AndroidRunner::adbKill(qint64 pid)
{
    {
        QProcess process;
        process.start(m_adb, selector() << _("shell")
            << _("kill") << QLatin1String("-9") << QString::number(pid));
        process.waitForFinished();
    }
    {
        QProcess process;
        process.start(m_adb, selector() << _("shell")
            << _("run-as") << m_packageName
            << _("kill") << QLatin1String("-9") << QString::number(pid));
        process.waitForFinished();
    }
}

QString AndroidRunner::displayName() const
{
    return m_packageName;
}

} // namespace Internal
} // namespace Android
