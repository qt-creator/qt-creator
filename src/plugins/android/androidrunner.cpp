/**************************************************************************
**
** Copyright (c) 2014 BogDan Vatra <bog_dan_ro@yahoo.com>
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
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

#include <QDir>
#include <QTime>
#include <QtConcurrentRun>
#include <QTemporaryFile>
#include <QTcpServer>

namespace Android {
namespace Internal {

typedef QLatin1String _;

AndroidRunner::AndroidRunner(QObject *parent,
                             AndroidRunConfiguration *runConfig,
                             ProjectExplorer::RunMode runMode)
    : QThread(parent)
{
    m_tries = 0;
    Debugger::DebuggerRunConfigurationAspect *aspect
            = runConfig->extraAspect<Debugger::DebuggerRunConfigurationAspect>();
    const bool debuggingMode = runMode == ProjectExplorer::DebugRunMode;
    m_useCppDebugger = debuggingMode && aspect->useCppDebugger();
    m_useQmlDebugger = debuggingMode && aspect->useQmlDebugger();
    QString channel = runConfig->remoteChannel();
    QTC_CHECK(channel.startsWith(QLatin1Char(':')));
    m_localGdbServerPort = channel.mid(1).toUShort();
    QTC_CHECK(m_localGdbServerPort);
    m_useQmlProfiler = runMode == ProjectExplorer::QmlProfilerRunMode;
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
}

AndroidRunner::~AndroidRunner()
{
    //stop();
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

        args << _("-e") << _("debug_ping") << _("true");
        args << _("-e") << _("ping_file") << m_pingFile;
        args << _("-e") << _("pong_file") << m_pongFile;
        args << _("-e") << _("gdbserver_command") << m_gdbserverCommand;
        args << _("-e") << _("gdbserver_socket") << m_gdbserverSocket;
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
    if (m_useLocalQtLibs) {
        args << _("-e") << _("use_local_qt_libs") << _("true");
        args << _("-e") << _("libs_prefix") << _("/data/local/tmp/qt/");
        args << _("-e") << _("load_local_libs") << m_localLibs;
        args << _("-e") << _("load_local_jars") << m_localJars;
        if (!m_localJarsInitClasses.isEmpty())
            args << _("-e") << _("static_init_classes") << m_localJarsInitClasses;
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

    m_tries = 0;
    m_wasStarted = false;
    QMetaObject::invokeMethod(&m_checkPIDTimer, "start");
}

void AndroidRunner::handleRemoteDebuggerRunning()
{
    if (m_useCppDebugger) {
        QTemporaryFile tmp(QDir::tempPath() + _("/pingpong"));
        tmp.open();

        QProcess process;
        process.start(m_adb, selector() << _("push") << tmp.fileName() << m_pongFile);
        process.waitForFinished();

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

    QByteArray pid(QString::fromLatin1("%1):").arg(m_processPID).toLatin1());
    foreach (QByteArray line, lines) {
        if (!line.contains(pid))
            continue;
        if (line.endsWith('\r'))
            line.chop(1);
        line.append('\n');
        if (onlyError || line.startsWith("F/")
                || line.startsWith("E/")
                || line.startsWith("D/Qt")
                || line.startsWith("W/"))
            emit remoteErrorOutput(line);
        else
            emit remoteOutput(line);

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
