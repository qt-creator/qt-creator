/****************************************************************************
**
** Copyright (C) 2018 BogDan Vatra <bog_dan_ro@yahoo.com>
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

#include "androidrunnerworker.h"

#include "androidconfigurations.h"
#include "androidconstants.h"
#include "androidmanager.h"
#include "androidrunconfiguration.h"

#include <debugger/debuggerrunconfigurationaspect.h>
#include <projectexplorer/target.h>
#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitinformation.h>
#include <utils/hostosinfo.h>
#include <utils/runextensions.h>
#include <utils/synchronousprocess.h>
#include <utils/temporaryfile.h>
#include <utils/url.h>

#include <QTcpServer>
#include <QThread>

#include <chrono>

using namespace std;
using namespace std::placeholders;
using namespace ProjectExplorer;

namespace Android {
namespace Internal {


static const QString pidScript = "pidof -s \"%1\"";
static const QString pidScriptPreNougat = QStringLiteral("for p in /proc/[0-9]*; "
                                                "do cat <$p/cmdline && echo :${p##*/}; done");
static const QString pidPollingScript = QStringLiteral("while [ -d /proc/%1 ]; do sleep 1; done");

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

static bool isTimedOut(const chrono::high_resolution_clock::time_point &start,
                            int msecs = APP_START_TIMEOUT)
{
    bool timedOut = false;
    auto end = chrono::high_resolution_clock::now();
    if (chrono::duration_cast<chrono::milliseconds>(end-start).count() > msecs)
        timedOut = true;
    return timedOut;
}

static qint64 extractPID(const QByteArray &output, const QString &packageName)
{
    qint64 pid = -1;
    foreach (auto tuple, output.split('\n')) {
        tuple = tuple.simplified();
        if (!tuple.isEmpty()) {
            auto parts = tuple.split(':');
            QString commandName = QString::fromLocal8Bit(parts.first());
            if (parts.length() == 2 && commandName == packageName) {
                pid = parts.last().toLongLong();
                break;
            }
        }
    }
    return pid;
}

static void findProcessPID(QFutureInterface<qint64> &fi, const QString &adbPath,
                           QStringList selector, const QString &packageName,
                           bool preNougat)
{
    if (packageName.isEmpty())
        return;

    qint64 processPID = -1;
    chrono::high_resolution_clock::time_point start = chrono::high_resolution_clock::now();

    selector.append("shell");
    selector.append(preNougat ? pidScriptPreNougat : pidScript.arg(packageName));

    do {
        QThread::msleep(200);
        const QByteArray out = Utils::SynchronousProcess().runBlocking(adbPath, selector).allRawOutput();
        if (preNougat) {
            processPID = extractPID(out, packageName);
        } else {
            if (!out.isEmpty())
                processPID = out.trimmed().toLongLong();
        }
    } while (processPID == -1 && !isTimedOut(start) && !fi.isCanceled());

    if (!fi.isCanceled())
        fi.reportResult(processPID);
}

static void deleter(QProcess *p)
{
    p->terminate();
    if (!p->waitForFinished(1000)) {
        p->kill();
        p->waitForFinished();
    }
    // Might get deleted from its own signal handler.
    p->deleteLater();
}

AndroidRunnerWorker::AndroidRunnerWorker(RunWorker *runner, const QString &packageName)
    : m_packageName(packageName)
    , m_adbLogcatProcess(nullptr, deleter)
    , m_psIsAlive(nullptr, deleter)
    , m_logCatRegExp(regExpLogcat)
    , m_gdbServerProcess(nullptr, deleter)
    , m_jdbProcess(nullptr, deleter)

{
    auto runConfig = runner->runControl()->runConfiguration();
    auto aspect = runConfig->extraAspect<Debugger::DebuggerRunConfigurationAspect>();
    Core::Id runMode = runner->runMode();
    const bool debuggingMode = runMode == ProjectExplorer::Constants::DEBUG_RUN_MODE;
    m_useCppDebugger = debuggingMode && aspect->useCppDebugger();
    if (debuggingMode && aspect->useQmlDebugger())
        m_qmlDebugServices = QmlDebug::QmlDebuggerServices;
    else if (runMode == ProjectExplorer::Constants::QML_PROFILER_RUN_MODE)
        m_qmlDebugServices = QmlDebug::QmlProfilerServices;
    else if (runMode == ProjectExplorer::Constants::QML_PREVIEW_RUN_MODE)
        m_qmlDebugServices = QmlDebug::QmlPreviewServices;
    else
        m_qmlDebugServices = QmlDebug::NoQmlDebugServices;
    m_localGdbServerPort = Utils::Port(5039);
    QTC_CHECK(m_localGdbServerPort.isValid());
    if (m_qmlDebugServices != QmlDebug::NoQmlDebugServices) {
        QTcpServer server;
        QTC_ASSERT(server.listen(QHostAddress::LocalHost),
                   qDebug() << tr("No free ports available on host for QML debugging."));
        m_qmlServer.setScheme(Utils::urlTcpScheme());
        m_qmlServer.setHost(server.serverAddress().toString());
        m_qmlServer.setPort(server.serverPort());
    }
    m_adb = AndroidConfigurations::currentConfig().adbToolPath().toString();
    m_localJdbServerPort = Utils::Port(5038);
    QTC_CHECK(m_localJdbServerPort.isValid());

    auto target = runConfig->target();
    m_deviceSerialNumber = AndroidManager::deviceSerialNumber(target);
    m_apiLevel = AndroidManager::deviceApiLevel(target);

    if (auto aspect = runConfig->extraAspect(Constants::ANDROID_AMSTARTARGS))
        m_amStartExtraArgs = static_cast<BaseStringAspect *>(aspect)->value().split(' ');

    if (auto aspect = runConfig->extraAspect(Constants::ANDROID_PRESTARTSHELLCMDLIST)) {
        for (const QString &shellCmd : static_cast<BaseStringListAspect *>(aspect)->value())
            m_beforeStartAdbCommands.append(QString("shell %1").arg(shellCmd));
    }
    for (const QString &shellCmd : runner->recordedData(Constants::ANDROID_PRESTARTSHELLCMDLIST).toStringList())
        m_beforeStartAdbCommands.append(QString("shell %1").arg(shellCmd));

    if (auto aspect = runConfig->extraAspect(Constants::ANDROID_POSTFINISHSHELLCMDLIST)) {
        for (const QString &shellCmd : static_cast<BaseStringListAspect *>(aspect)->value())
            m_afterFinishAdbCommands.append(QString("shell %1").arg(shellCmd));
    }
    for (const QString &shellCmd : runner->recordedData(Constants::ANDROID_POSTFINISHSHELLCMDLIST).toStringList())
        m_afterFinishAdbCommands.append(QString("shell %1").arg(shellCmd));
}

AndroidRunnerWorker::~AndroidRunnerWorker()
{
    if (m_processPID != -1)
        forceStop();

    if (!m_pidFinder.isFinished())
        m_pidFinder.cancel();
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

bool AndroidRunnerWorker::runAdb(const QStringList &args, int timeoutS)
{
    Utils::SynchronousProcess adb;
    adb.setTimeoutS(timeoutS);
    Utils::SynchronousProcessResponse response = adb.run(m_adb, selector() + args);
    m_lastRunAdbError = response.exitMessage(m_adb, timeoutS);
    m_lastRunAdbRawOutput = response.allRawOutput();
    return response.result == Utils::SynchronousProcessResponse::Finished;
}

void AndroidRunnerWorker::adbKill(qint64 pid)
{
    runAdb({"shell", "kill", "-9", QString::number(pid)});
    runAdb({"shell", "run-as", m_packageName, "kill", "-9", QString::number(pid)});
}

QStringList AndroidRunnerWorker::selector() const
{
    return AndroidDeviceInfo::adbSelector(m_deviceSerialNumber);
}

void AndroidRunnerWorker::forceStop()
{
    runAdb({"shell", "am", "force-stop", m_packageName}, 30);

    // try killing it via kill -9
    const QByteArray out = Utils::SynchronousProcess()
            .runBlocking(m_adb, selector() << QStringLiteral("shell") << pidScriptPreNougat)
            .allRawOutput();

    qint64 pid = extractPID(out.simplified(), m_packageName);
    if (pid != -1) {
        adbKill(pid);
    }
}

void AndroidRunnerWorker::logcatReadStandardError()
{
    if (m_processPID != -1)
        logcatProcess(m_adbLogcatProcess->readAllStandardError(), m_stderrBuffer, true);
}

void AndroidRunnerWorker::logcatReadStandardOutput()
{
    if (m_processPID != -1)
        logcatProcess(m_adbLogcatProcess->readAllStandardOutput(), m_stdoutBuffer, false);
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

    QString pidString = QString::number(m_processPID);
    foreach (const QByteArray &msg, lines) {
        const QString line = QString::fromUtf8(msg).trimmed() + QLatin1Char('\n');
        if (!line.contains(pidString))
            continue;
        if (m_useCppDebugger) {
            switch (m_jdbState) {
            case JDBState::Idle:
                if (msg.trimmed().endsWith("Sending WAIT chunk")) {
                    m_jdbState = JDBState::Waiting;
                    handleJdbWaiting();
                }
                break;
            case JDBState::Waiting:
                if (msg.indexOf("debugger has settled") > 0) {
                    m_jdbState = JDBState::Settled;
                    handleJdbSettled();
                }
                break;
            default:
                break;
            }
        }
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
            if (onlyError || line.startsWith("F/")
                    || line.startsWith("E/")
                    || line.startsWith("W/"))
                emit remoteErrorOutput(line);
            else
                emit remoteOutput(line);
        }
    }
}

void AndroidRunnerWorker::setAndroidDeviceInfo(const AndroidDeviceInfo &info)
{
    m_deviceSerialNumber = info.serialNumber;
    m_apiLevel = info.sdk;
}

void AndroidRunnerWorker::asyncStartHelper()
{
    forceStop();

    // Start the logcat process before app starts.
    std::unique_ptr<QProcess, Deleter> logcatProcess(new QProcess, deleter);
    connect(logcatProcess.get(), &QProcess::readyReadStandardOutput,
            this, &AndroidRunnerWorker::logcatReadStandardOutput);
    connect(logcatProcess.get(), &QProcess::readyReadStandardError,
            this, &AndroidRunnerWorker::logcatReadStandardError);
    // Its assumed that the device or avd returned by selector() is online.
    logcatProcess->start(m_adb, selector() << "logcat");
    QTC_ASSERT(!m_adbLogcatProcess, /**/);
    m_adbLogcatProcess = std::move(logcatProcess);

    for (const QString &entry : m_beforeStartAdbCommands)
        runAdb(entry.split(' ', QString::SkipEmptyParts));

    QStringList args({"shell", "am", "start"});
    args << m_amStartExtraArgs;
    args << "-n" << m_intentName;
    if (m_useCppDebugger) {
        args << "-D";
        QString gdbServerSocket;
        // run-as <package-name> pwd fails on API 22 so route the pwd through shell.
        if (!runAdb({"shell", "run-as", m_packageName, "/system/bin/sh", "-c", "pwd"})) {
            emit remoteProcessFinished(tr("Failed to get process path. Reason: %1.").arg(m_lastRunAdbError));
            return;
        }
        gdbServerSocket = QString::fromUtf8(m_lastRunAdbRawOutput.trimmed()) + "/debug-socket";

        QString gdbServerExecutable;
        if (!runAdb({"shell", "run-as", m_packageName, "ls", "lib/"})) {
            emit remoteProcessFinished(tr("Failed to get process path. Reason: %1.").arg(m_lastRunAdbError));
            return;
        }

        for (const auto &line: m_lastRunAdbRawOutput.split('\n')) {
            if (line.indexOf("gdbserver") != -1/* || line.indexOf("lldb-server") != -1*/) {
                gdbServerExecutable = QString::fromUtf8(line.trimmed());
                break;
            }
        }

        if (gdbServerExecutable.isEmpty()) {
            emit remoteProcessFinished(tr("Cannot find C++ debugger."));
            return;
        }

        runAdb({"shell", "run-as", m_packageName, "killall", gdbServerExecutable});
        runAdb({"shell", "run-as", m_packageName, "rm", gdbServerSocket});
        std::unique_ptr<QProcess, Deleter> gdbServerProcess(new QProcess, deleter);
        gdbServerProcess->start(m_adb, selector() << "shell" << "run-as"
                                    << m_packageName << "lib/" + gdbServerExecutable
                                    << "--multi" << "+" + gdbServerSocket);
        if (!gdbServerProcess->waitForStarted()) {
            emit remoteProcessFinished(tr("Failed to start C++ debugger."));
            return;
        }
        m_gdbServerProcess = std::move(gdbServerProcess);
        QStringList removeForward{"forward", "--remove", "tcp:" + m_localGdbServerPort.toString()};
        runAdb(removeForward);
        if (!runAdb({"forward", "tcp:" + m_localGdbServerPort.toString(),
                    "localfilesystem:" + gdbServerSocket})) {
            emit remoteProcessFinished(tr("Failed to forward C++ debugging ports. Reason: %1.").arg(m_lastRunAdbError));
            return;
        }
        m_afterFinishAdbCommands.push_back(removeForward.join(' '));
    }

    if (m_qmlDebugServices != QmlDebug::NoQmlDebugServices) {
        // currently forward to same port on device and host
        const QString port = QString("tcp:%1").arg(m_qmlServer.port());
        QStringList removeForward{{"forward", "--remove", port}};
        runAdb(removeForward);
        if (!runAdb({"forward", port, port})) {
            emit remoteProcessFinished(tr("Failed to forward QML debugging ports. Reason: %1.")
                                       .arg(m_lastRunAdbError) + "\n" + m_lastRunAdbRawOutput);
            return;
        }
        m_afterFinishAdbCommands.push_back(removeForward.join(' '));

        args << "-e" << "qml_debug" << "true"
             << "-e" << "qmljsdebugger"
             << QString("port:%1,block,services:%2")
                .arg(m_qmlServer.port()).arg(QmlDebug::qmlDebugServices(m_qmlDebugServices));
    }

    if (!m_extraAppParams.isEmpty()) {
        args << "-e" << "extraappparams"
             << QString::fromLatin1(m_extraAppParams.toUtf8().toBase64());
    }

    if (m_extraEnvVars.size() > 0) {
        args << "-e" << "extraenvvars"
             << QString::fromLatin1(m_extraEnvVars.toStringList().join('\t')
                                    .toUtf8().toBase64());
    }

    if (!runAdb(args)) {
        emit remoteProcessFinished(tr("Failed to start the activity. Reason: %1.")
                                   .arg(m_lastRunAdbError));
        return;
    }
}

void AndroidRunnerWorker::asyncStart()
{
    asyncStartHelper();

    m_pidFinder = Utils::onResultReady(Utils::runAsync(findProcessPID, m_adb, selector(),
                                                       m_packageName, m_isPreNougat),
                                       bind(&AndroidRunnerWorker::onProcessIdChanged, this, _1));
}

void AndroidRunnerWorker::asyncStop()
{
    if (!m_pidFinder.isFinished())
        m_pidFinder.cancel();

    if (m_processPID != -1)
        forceStop();

    m_jdbProcess.reset();
    m_gdbServerProcess.reset();
}

void AndroidRunnerWorker::handleJdbWaiting()
{
    QStringList removeForward{"forward", "--remove", "tcp:" + m_localJdbServerPort.toString()};
    runAdb(removeForward);
    if (!runAdb({"forward", "tcp:" + m_localJdbServerPort.toString(),
                "jdwp:" + QString::number(m_processPID)})) {
        emit remoteProcessFinished(tr("Failed to forward jdb debugging ports. Reason: %1.").arg(m_lastRunAdbError));
        return;
    }
    m_afterFinishAdbCommands.push_back(removeForward.join(' '));

    auto jdbPath = AndroidConfigurations::currentConfig().openJDKLocation().appendPath("bin");
    if (Utils::HostOsInfo::isWindowsHost())
        jdbPath.appendPath("jdb.exe");
    else
        jdbPath.appendPath("jdb");

    std::unique_ptr<QProcess, Deleter> jdbProcess(new QProcess, &deleter);
    jdbProcess->setProcessChannelMode(QProcess::MergedChannels);
    jdbProcess->start(jdbPath.toString(), QStringList() << "-connect" <<
                      QString("com.sun.jdi.SocketAttach:hostname=localhost,port=%1")
                      .arg(m_localJdbServerPort.toString()));
    if (!jdbProcess->waitForStarted()) {
        emit remoteProcessFinished(tr("Failed to start jdb"));
        return;
    }
    m_jdbProcess = std::move(jdbProcess);
}

void AndroidRunnerWorker::handleJdbSettled()
{
    auto waitForCommand = [&]() {
        for (int i= 0; i < 5 && m_jdbProcess->state() == QProcess::Running; ++i) {
            m_jdbProcess->waitForReadyRead(500);
            QByteArray lines = m_jdbProcess->readAll();
            for (const auto &line: lines.split('\n')) {
                auto msg = line.trimmed();
                if (msg.startsWith(">"))
                    return true;
            }
        }
        return false;
    };
    if (waitForCommand()) {
        m_jdbProcess->write("cont\n");
        if (m_jdbProcess->waitForBytesWritten(5000) && waitForCommand()) {
            m_jdbProcess->write("exit\n");
            m_jdbProcess->waitForBytesWritten(5000);
            if (!m_jdbProcess->waitForFinished(5000)) {
                m_jdbProcess->terminate();
                if (!m_jdbProcess->waitForFinished(5000)) {
                    m_jdbProcess->kill();
                    m_jdbProcess->waitForFinished();
                }
            } else if (m_jdbProcess->exitStatus() == QProcess::NormalExit && m_jdbProcess->exitCode() == 0) {
                return;
            }
        }
    }
    emit remoteProcessFinished(tr("Cannot attach jdb to the running application").arg(m_lastRunAdbError));
}

void AndroidRunnerWorker::onProcessIdChanged(qint64 pid)
{
    // Don't write to m_psProc from a different thread
    QTC_ASSERT(QThread::currentThread() == thread(), return);
    m_processPID = pid;
    if (pid == -1) {
        emit remoteProcessFinished(QLatin1String("\n\n") + tr("\"%1\" died.")
                                   .arg(m_packageName));
        // App died/killed. Reset log, monitor, jdb & gdb processes.
        m_adbLogcatProcess.reset();
        m_psIsAlive.reset();
        m_jdbProcess.reset();
        m_gdbServerProcess.reset();

        // Run adb commands after application quit.
        for (const QString &entry: m_afterFinishAdbCommands)
            runAdb(entry.split(' ', QString::SkipEmptyParts));
    } else {
        // In debugging cases this will be funneled to the engine to actually start
        // and attach gdb. Afterwards this ends up in handleRemoteDebuggerRunning() below.
        emit remoteProcessStarted(m_localGdbServerPort, m_qmlServer, m_processPID);
        logcatReadStandardOutput();
        QTC_ASSERT(!m_psIsAlive, /**/);
        m_psIsAlive.reset(new QProcess);
        m_psIsAlive->setProcessChannelMode(QProcess::MergedChannels);
        connect(m_psIsAlive.get(), static_cast<void(QProcess::*)(int)>(&QProcess::finished),
                this, bind(&AndroidRunnerWorker::onProcessIdChanged, this, -1));
        m_psIsAlive->start(m_adb, selector() << QStringLiteral("shell")
                           << pidPollingScript.arg(m_processPID));
    }
}

void AndroidRunnerWorker::setExtraEnvVars(const Utils::Environment &extraEnvVars)
{
    m_extraEnvVars = extraEnvVars;
}

void AndroidRunnerWorker::setExtraAppParams(const QString &extraAppParams)
{
    m_extraAppParams = extraAppParams;
}


} // namespace Internal
} // namespace Android
