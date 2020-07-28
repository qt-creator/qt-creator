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

#include <debugger/debuggerkitinformation.h>
#include <debugger/debuggerrunconfigurationaspect.h>

#include <projectexplorer/environmentaspect.h>
#include <projectexplorer/runconfigurationaspects.h>
#include <projectexplorer/runcontrol.h>
#include <projectexplorer/target.h>

#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitinformation.h>

#include <utils/fileutils.h>
#include <utils/hostosinfo.h>
#include <utils/qtcprocess.h>
#include <utils/runextensions.h>
#include <utils/stringutils.h>
#include <utils/synchronousprocess.h>
#include <utils/temporaryfile.h>
#include <utils/url.h>

#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QLoggingCategory>
#include <QScopeGuard>
#include <QRegularExpression>
#include <QTcpServer>
#include <QThread>

#include <chrono>

namespace {
static Q_LOGGING_CATEGORY(androidRunWorkerLog, "qtc.android.run.androidrunnerworker", QtWarningMsg)
static const int GdbTempFileMaxCounter = 20;
}

using namespace std;
using namespace std::placeholders;
using namespace ProjectExplorer;
using namespace Utils;

namespace Android {
namespace Internal {

static const QString pidScript = "pidof -s '%1'";
static const QString pidScriptPreNougat = QStringLiteral("for p in /proc/[0-9]*; "
                                                "do cat <$p/cmdline && echo :${p##*/}; done");
static const QString pidPollingScript = QStringLiteral("while [ -d /proc/%1 ]; do sleep 1; done");

static const QRegularExpression regExpLogcat{"^[0-9\\-]*" // date
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
                                             "[\\n\\r]*$"};
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

static void findProcessPID(QFutureInterface<qint64> &fi, QStringList selector,
                           const QString &packageName, bool preNougat)
{
    if (packageName.isEmpty())
        return;

    qint64 processPID = -1;
    chrono::high_resolution_clock::time_point start = chrono::high_resolution_clock::now();
    do {
        QThread::msleep(200);
        FilePath adbPath = AndroidConfigurations::currentConfig().adbToolPath();
        selector.append("shell");
        selector.append(preNougat ? pidScriptPreNougat : pidScript.arg(packageName));
        const auto out = SynchronousProcess().runBlocking({adbPath, selector}).allRawOutput();
        if (preNougat) {
            processPID = extractPID(out, packageName);
        } else {
            if (!out.isEmpty())
                processPID = out.trimmed().toLongLong();
        }
    } while (processPID == -1 && !isTimedOut(start) && !fi.isCanceled());

    qCDebug(androidRunWorkerLog) << "PID found:" << processPID << ", PreNougat:" << preNougat;
    if (!fi.isCanceled())
        fi.reportResult(processPID);
}

static void deleter(QProcess *p)
{
    qCDebug(androidRunWorkerLog) << "Killing process:" << p->objectName();
    p->terminate();
    if (!p->waitForFinished(1000)) {
        p->kill();
        p->waitForFinished();
    }
    // Might get deleted from its own signal handler.
    p->deleteLater();
}

static QString gdbServerArch(const QString &androidAbi)
{
    if (androidAbi == "arm64-v8a")
        return QString("arm64");
    if (androidAbi == "armeabi-v7a")
        return QString("arm");
    // That's correct for "x86_64" and "x86", and best guess at anything that will evolve:
    return androidAbi;
}

static QString lldbServerArch(const QString &androidAbi)
{
    if (androidAbi == "armeabi-v7a")
        return QString("armeabi");
    // Correct for arm64-v8a "x86_64" and "x86", and best guess at anything that will evolve:
    return androidAbi; // arm64-v8a, x86, x86_64
}

static QString lldbServerArch2(const QString &androidAbi)
{
    if (androidAbi == "armeabi-v7a")
        return {"arm"};
    if (androidAbi == "x86")
        return {"i386"};
    if (androidAbi == "arm64-v8a")
        return {"aarch64"};
    // Correct for "x86_64" a and best guess at anything that will evolve:
    return androidAbi; // arm64-v8a
}

static FilePath debugServer(bool useLldb, const Target *target)
{
    QtSupport::BaseQtVersion *qtVersion = QtSupport::QtKitAspect::qtVersion(target->kit());
    QString preferredAbi = AndroidManager::apkDevicePreferredAbi(target);

    const AndroidConfig &config = AndroidConfigurations::currentConfig();

    if (useLldb) {
        // Search suitable lldb-server binary.
        const FilePath prebuilt = config.ndkLocation(qtVersion) / "toolchains/llvm/prebuilt";
        const QString abiNeedle = lldbServerArch2(preferredAbi);

        // The new, built-in LLDB.
        QDirIterator it(prebuilt.toString(), QDir::Files|QDir::Executable, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            it.next();
            const QString filePath = it.filePath();
            if (filePath.endsWith(abiNeedle + "/lldb-server")) {
                return FilePath::fromString(filePath);
            }
        }

        // Older: Find LLDB version. sdk_definitions.json contains something like  "lldb;3.1". Use that.
        const QStringList packages = config.defaultEssentials();
        for (const QString &package : packages) {
            if (package.startsWith("lldb;")) {
                const QString lldbVersion = package.mid(5);
                const FilePath path = config.sdkLocation()
                        / QString("lldb/%1/android/%2/lldb-server")
                                .arg(lldbVersion, lldbServerArch(preferredAbi));
                if (path.exists())
                    return path;
            }
        }
    } else {
        // Search suitable gdbserver binary.
        const FilePath path = config.ndkLocation(qtVersion)
                .pathAppended(QString("prebuilt/android-%1/gdbserver/gdbserver")
                              .arg(gdbServerArch(preferredAbi)));
        if (path.exists())
            return path;
    }

    return {};
}


AndroidRunnerWorker::AndroidRunnerWorker(RunWorker *runner, const QString &packageName)
    : m_packageName(packageName)
    , m_adbLogcatProcess(nullptr, deleter)
    , m_psIsAlive(nullptr, deleter)
    , m_debugServerProcess(nullptr, deleter)
    , m_jdbProcess(nullptr, deleter)

{
    auto runControl = runner->runControl();
    m_useLldb = Debugger::DebuggerKitAspect::engineType(runControl->kit())
                    == Debugger::LldbEngineType;
    auto aspect = runControl->aspect<Debugger::DebuggerRunConfigurationAspect>();
    Utils::Id runMode = runControl->runMode();
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
    m_localDebugServerPort = Utils::Port(5039);
    QTC_CHECK(m_localDebugServerPort.isValid());
    if (m_qmlDebugServices != QmlDebug::NoQmlDebugServices) {
        qCDebug(androidRunWorkerLog) << "QML debugging enabled";
        QTcpServer server;
        QTC_ASSERT(server.listen(QHostAddress::LocalHost),
                   qDebug() << tr("No free ports available on host for QML debugging."));
        m_qmlServer.setScheme(Utils::urlTcpScheme());
        m_qmlServer.setHost(server.serverAddress().toString());
        m_qmlServer.setPort(server.serverPort());
        qCDebug(androidRunWorkerLog) << "QML server:" << m_qmlServer.toDisplayString();
    }
    m_localJdbServerPort = Utils::Port(5038);
    QTC_CHECK(m_localJdbServerPort.isValid());

    auto target = runControl->target();
    m_deviceSerialNumber = AndroidManager::deviceSerialNumber(target);
    m_apiLevel = AndroidManager::deviceApiLevel(target);

    m_extraEnvVars = runControl->aspect<EnvironmentAspect>()->environment();
    qCDebug(androidRunWorkerLog) << "Environment variables for the app"
                                 << m_extraEnvVars.toStringList();

    m_extraAppParams = runControl->runnable().commandLineArguments;

    if (auto aspect = runControl->aspect(Constants::ANDROID_AMSTARTARGS)) {
        const QString startArgs = static_cast<BaseStringAspect *>(aspect)->value();
        m_amStartExtraArgs = QtcProcess::splitArgs(startArgs, OsTypeOtherUnix);
    }

    if (auto aspect = runControl->aspect(Constants::ANDROID_PRESTARTSHELLCMDLIST)) {
        for (const QString &shellCmd : static_cast<BaseStringListAspect *>(aspect)->value())
            m_beforeStartAdbCommands.append(QString("shell %1").arg(shellCmd));
    }
    for (const QString &shellCmd : runner->recordedData(Constants::ANDROID_PRESTARTSHELLCMDLIST).toStringList())
        m_beforeStartAdbCommands.append(QString("shell %1").arg(shellCmd));

    if (auto aspect = runControl->aspect(Constants::ANDROID_POSTFINISHSHELLCMDLIST)) {
        for (const QString &shellCmd : static_cast<BaseStringListAspect *>(aspect)->value())
            m_afterFinishAdbCommands.append(QString("shell %1").arg(shellCmd));
    }
    for (const QString &shellCmd : runner->recordedData(Constants::ANDROID_POSTFINISHSHELLCMDLIST).toStringList())
        m_afterFinishAdbCommands.append(QString("shell %1").arg(shellCmd));

    m_debugServerPath = debugServer(m_useLldb, target).toString();
    qCDebug(androidRunWorkerLog) << "Device Serial:" << m_deviceSerialNumber
                                 << ", API level:" << m_apiLevel
                                 << ", Extra Start Args:" << m_amStartExtraArgs
                                 << ", Before Start ADB cmds:" << m_beforeStartAdbCommands
                                 << ", After finish ADB cmds:" << m_afterFinishAdbCommands
                                 << ", Debug server path:" << m_debugServerPath;

    QtSupport::BaseQtVersion *version = QtSupport::QtKitAspect::qtVersion(target->kit());
    m_useAppParamsForQmlDebugger = version->qtVersion() >= QtSupport::QtVersionNumber(5, 12);
}

AndroidRunnerWorker::~AndroidRunnerWorker()
{
    if (m_processPID != -1)
        forceStop();

    if (!m_pidFinder.isFinished())
        m_pidFinder.cancel();
}

bool AndroidRunnerWorker::runAdb(const QStringList &args, QString *stdOut,
                                 const QByteArray &writeData)
{
    QStringList adbArgs = selector() + args;
    SdkToolResult result = AndroidManager::runAdbCommand(adbArgs, writeData);
    if (!result.success())
        emit remoteErrorOutput(result.stdErr());
    if (stdOut)
        *stdOut = result.stdOut();
    return result.success();
}

bool AndroidRunnerWorker::uploadDebugServer(const QString &debugServerFileName)
{
    // Push the gdbserver or lldb-server to  temp location and then to package dir.
    // the files can't be pushed directly to package because of permissions.
    qCDebug(androidRunWorkerLog) << "Uploading GdbServer";

    // Get a unique temp file name for gdb/lldbserver copy
    const QString tempDebugServerPathTemplate = "/data/local/tmp/%1";
    int count = 0;
    while (deviceFileExists(tempDebugServerPathTemplate.arg(++count))) {
        if (count > GdbTempFileMaxCounter) {
            qCDebug(androidRunWorkerLog) << "Can not get temporary file name";
            return false;
        }
    }

    const QString tempDebugServerPath = tempDebugServerPathTemplate.arg(count);
    auto cleanUp = qScopeGuard([this, tempDebugServerPath] {
        if (!runAdb({"shell", "rm", "-f", tempDebugServerPath}))
            qCDebug(androidRunWorkerLog) << "Debug server cleanup failed.";
    });

    // Copy gdbserver to temp location
    if (!runAdb({"push", m_debugServerPath , tempDebugServerPath})) {
        qCDebug(androidRunWorkerLog) << "Debug server upload to temp directory failed";
        return false;
    }

    // Copy gdbserver from temp location to app directory
    if (!runAdb({"shell", "run-as", m_packageName, "cp" , tempDebugServerPath, debugServerFileName})) {
        qCDebug(androidRunWorkerLog) << "Debug server copy from temp directory failed";
        return false;
    }
    QTC_ASSERT(runAdb({"shell", "run-as", m_packageName, "chmod", "777", debugServerFileName}),
                   qCDebug(androidRunWorkerLog) << "Debug server chmod 777 failed.");
    return true;
}

bool AndroidRunnerWorker::deviceFileExists(const QString &filePath)
{
    QString output;
    const bool success = runAdb({"shell", "ls", filePath, "2>/dev/null"}, &output);
    return success && !output.trimmed().isEmpty();
}

bool AndroidRunnerWorker::packageFileExists(const QString &filePath)
{
    QString output;
    const bool success = runAdb({"shell", "run-as", m_packageName, "ls", filePath, "2>/dev/null"}, &output);
    return success && !output.trimmed().isEmpty();
}

void AndroidRunnerWorker::adbKill(qint64 pid)
{
    if (!runAdb({"shell", "run-as", m_packageName, "kill", "-9", QString::number(pid)}))
        runAdb({"shell", "kill", "-9", QString::number(pid)});
}

QStringList AndroidRunnerWorker::selector() const
{
    return AndroidDeviceInfo::adbSelector(m_deviceSerialNumber);
}

void AndroidRunnerWorker::forceStop()
{
    runAdb({"shell", "am", "force-stop", m_packageName});

    // try killing it via kill -9
    QString output;
    runAdb({"shell", "pidof", m_packageName}, &output);
    if (m_processPID != -1 && output == QString::number(m_processPID))
        adbKill(m_processPID);
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
        const QRegularExpressionMatch match = regExpLogcat.match(line);
        if (match.hasMatch()) {
            // Android M
            if (match.captured(1) == pidString) {
                const QString messagetype = match.captured(2);
                const QString output = line.mid(match.capturedStart(2));

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
    qCDebug(androidRunWorkerLog) << "Android Device Info changed"
                                 << m_deviceSerialNumber << m_apiLevel;
}

void AndroidRunnerWorker::asyncStartHelper()
{
    forceStop();

    // Its assumed that the device or avd returned by selector() is online.
    // Start the logcat process before app starts.
    QTC_ASSERT(!m_adbLogcatProcess, /**/);
    m_adbLogcatProcess.reset(AndroidManager::runAdbCommandDetached(selector() << "logcat"));
    if (m_adbLogcatProcess) {
        m_adbLogcatProcess->setObjectName("AdbLogcatProcess");
        connect(m_adbLogcatProcess.get(), &QProcess::readyReadStandardOutput,
                this, &AndroidRunnerWorker::logcatReadStandardOutput);
        connect(m_adbLogcatProcess.get(), &QProcess::readyReadStandardError,
                this, &AndroidRunnerWorker::logcatReadStandardError);
    }

    for (const QString &entry : m_beforeStartAdbCommands)
        runAdb(entry.split(' ', Qt::SkipEmptyParts));

    QStringList args({"shell", "am", "start"});
    args << m_amStartExtraArgs;
    args << "-n" << m_intentName;
    if (m_useCppDebugger) {
        args << "-D";
        // run-as <package-name> pwd fails on API 22 so route the pwd through shell.
        QString packageDir;
        if (!runAdb({"shell", "run-as", m_packageName, "/system/bin/sh", "-c", "pwd"},
                    &packageDir)) {
            emit remoteProcessFinished(tr("Failed to find application directory."));
            return;
        }

        // Add executable flag to package dir. Gdb can't connect to running server on device on
        // e.g. on Android 8 with NDK 10e
        runAdb({"shell", "run-as", m_packageName, "chmod", "a+x", packageDir.trimmed()});

        if (!QFileInfo::exists(m_debugServerPath)) {
            QString msg = tr("Cannot find C++ debug server in NDK installation.");
            if (m_useLldb)
                msg += "\n" + tr("The lldb-server binary has not been found.");
            emit remoteProcessFinished(msg);
            return;
        }

        QString debugServerFile;
        if (m_useLldb) {
            debugServerFile = "./lldb-server";
            runAdb({"shell", "run-as", m_packageName, "killall", "lldb-server"});
            if (!uploadDebugServer(debugServerFile)) {
                emit remoteProcessFinished(tr("Cannot copy C++ debug server."));
                return;
            }
        } else {
            if (packageFileExists("./lib/gdbserver")) {
                debugServerFile = "./lib/gdbserver";
                qCDebug(androidRunWorkerLog) << "Found GDB server " + debugServerFile;
                runAdb({"shell", "run-as", m_packageName, "killall", "gdbserver"});
            } else if (packageFileExists("./lib/libgdbserver.so")) {
                debugServerFile = "./lib/libgdbserver.so";
                qCDebug(androidRunWorkerLog) << "Found GDB server " + debugServerFile;
                runAdb({"shell", "run-as", m_packageName, "killall", "libgdbserver.so"});
            } else {
                // Armv8. symlink lib is not available.
                debugServerFile = "./gdbserver";
                // Kill the previous instances of gdbserver. Do this before copying the gdbserver.
                runAdb({"shell", "run-as", m_packageName, "killall", "gdbserver"});
                if (!uploadDebugServer("./gdbserver")) {
                    emit remoteProcessFinished(tr("Cannot copy C++ debug server."));
                    return;
                }
            }
        }
        QString debuggerServerErr;
        if (!startDebuggerServer(packageDir, debugServerFile, &debuggerServerErr)) {
            emit remoteProcessFinished(debuggerServerErr);
            return;
        }
    }

    if (m_qmlDebugServices != QmlDebug::NoQmlDebugServices) {
        // currently forward to same port on device and host
        const QString port = QString("tcp:%1").arg(m_qmlServer.port());
        QStringList removeForward{{"forward", "--remove", port}};
        removeForwardPort(port);
        if (!runAdb({"forward", port, port})) {
            emit remoteProcessFinished(tr("Failed to forward QML debugging ports."));
            return;
        }
        m_afterFinishAdbCommands.push_back(removeForward.join(' '));

        const QString qmljsdebugger = QString("port:%1,block,services:%2")
                .arg(m_qmlServer.port()).arg(QmlDebug::qmlDebugServices(m_qmlDebugServices));

        if (m_useAppParamsForQmlDebugger) {
            if (!m_extraAppParams.isEmpty())
                m_extraAppParams.prepend(' ');
            m_extraAppParams.prepend("-qmljsdebugger=" + qmljsdebugger);
        } else {
            args << "-e" << "qml_debug" << "true"
                 << "-e" << "qmljsdebugger"
                 << qmljsdebugger;
        }
    }


    if (!m_extraAppParams.isEmpty()) {
        QStringList appArgs =
                Utils::QtcProcess::splitArgs(m_extraAppParams, Utils::OsType::OsTypeLinux);
        qCDebug(androidRunWorkerLog) << "Using application arguments: " << appArgs;
        args << "-e" << "extraappparams"
             << QString::fromLatin1(appArgs.join(' ').toUtf8().toBase64());
    }

    if (m_extraEnvVars.size() > 0) {
        args << "-e" << "extraenvvars"
             << QString::fromLatin1(m_extraEnvVars.toStringList().join('\t')
                                    .toUtf8().toBase64());
    }

    if (!runAdb(args)) {
        emit remoteProcessFinished(tr("Failed to start the activity."));
        return;
    }
}

bool AndroidRunnerWorker::startDebuggerServer(const QString &packageDir,
                                              const QString &debugServerFile,
                                              QString *errorStr)
{
    if (m_useLldb) {
        QString lldbServerErr;
        QStringList lldbServerArgs = selector();
        lldbServerArgs << "shell" << "run-as" << m_packageName << debugServerFile
                        << "platform"
                        // << "--server"  // Can lead to zombie servers
                        << "--listen" << QString("*:%1").arg(m_localDebugServerPort.toString());
        m_debugServerProcess.reset(AndroidManager::runAdbCommandDetached(lldbServerArgs, &lldbServerErr));

        if (!m_debugServerProcess) {
            qCDebug(androidRunWorkerLog) << "Debugger process failed to start" << lldbServerErr;
            if (errorStr)
                *errorStr = tr("Failed to start debugger server.");
            return false;
        }
        qCDebug(androidRunWorkerLog) << "Debugger process started";
        m_debugServerProcess->setObjectName("AndroidDebugServerProcess");

    } else {
        QString gdbServerSocket = packageDir + "/debug-socket";
        runAdb({"shell", "run-as", m_packageName, "rm", gdbServerSocket});

        QString gdbProcessErr;
        QStringList gdbServerErr = selector();
        gdbServerErr << "shell" << "run-as" << m_packageName << debugServerFile
                      << "--multi" << "+" + gdbServerSocket;
        m_debugServerProcess.reset(AndroidManager::runAdbCommandDetached(gdbServerErr, &gdbProcessErr));

        if (!m_debugServerProcess) {
            qCDebug(androidRunWorkerLog) << "Debugger process failed to start" << gdbServerErr;
            if (errorStr)
                *errorStr = tr("Failed to start debugger server.");
            return false;
        }
        qCDebug(androidRunWorkerLog) << "Debugger process started";
        m_debugServerProcess->setObjectName("AndroidDebugServerProcess");

        const QString port = "tcp:" + m_localDebugServerPort.toString();
        const QStringList removeForward{"forward", "--remove", port};
        removeForwardPort(port);
        if (!runAdb({"forward", port,
                    "localfilesystem:" + gdbServerSocket})) {
            if (errorStr)
                *errorStr = tr("Failed to forward C++ debugging ports.");
            return false;
        }
        m_afterFinishAdbCommands.push_back(removeForward.join(' '));
    }
    return true;
}

void AndroidRunnerWorker::asyncStart()
{
    asyncStartHelper();

    m_pidFinder = Utils::onResultReady(Utils::runAsync(findProcessPID, selector(),
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
    m_debugServerProcess.reset();
}

void AndroidRunnerWorker::handleJdbWaiting()
{
    const QString port = "tcp:" + m_localJdbServerPort.toString();
    const QStringList removeForward{"forward", "--remove", port};
    removeForwardPort(port);
    if (!runAdb({"forward", port,
                "jdwp:" + QString::number(m_processPID)})) {
        emit remoteProcessFinished(tr("Failed to forward JDB debugging ports."));
        return;
    }
    m_afterFinishAdbCommands.push_back(removeForward.join(' '));

    auto jdbPath = AndroidConfigurations::currentConfig().openJDKLocation().pathAppended("bin");
    jdbPath = jdbPath.pathAppended(Utils::HostOsInfo::withExecutableSuffix("jdb"));

    QStringList jdbArgs("-connect");
    jdbArgs << QString("com.sun.jdi.SocketAttach:hostname=localhost,port=%1")
               .arg(m_localJdbServerPort.toString());
    qCDebug(androidRunWorkerLog) << "Starting JDB:" << CommandLine(jdbPath, jdbArgs).toUserOutput();
    std::unique_ptr<QProcess, Deleter> jdbProcess(new QProcess, &deleter);
    jdbProcess->setProcessChannelMode(QProcess::MergedChannels);
    jdbProcess->start(jdbPath.toString(), jdbArgs);
    if (!jdbProcess->waitForStarted()) {
        emit remoteProcessFinished(tr("Failed to start JDB."));
        return;
    }
    m_jdbProcess = std::move(jdbProcess);
    m_jdbProcess->setObjectName("JdbProcess");
}

void AndroidRunnerWorker::handleJdbSettled()
{
    qCDebug(androidRunWorkerLog) << "Handle JDB settled";
    auto waitForCommand = [this]() {
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
                    qCDebug(androidRunWorkerLog) << "Killing JDB process";
                    m_jdbProcess->kill();
                    m_jdbProcess->waitForFinished();
                }
            } else if (m_jdbProcess->exitStatus() == QProcess::NormalExit && m_jdbProcess->exitCode() == 0) {
                qCDebug(androidRunWorkerLog) << "JDB settled";
                return;
            }
        }
    }
    emit remoteProcessFinished(tr("Cannot attach JDB to the running application."));
}

void AndroidRunnerWorker::removeForwardPort(const QString &port)
{
    bool found = false;
    SdkToolResult result = AndroidManager::runAdbCommand({"forward", "--list"});

    QString string = result.stdOut();
    for (const QString &line : string.split('\n')) {
        if (line.contains(port)) {
            found = true;
            break;
        }
    }

    if (found) {
        QStringList removeForward{"forward", "--remove", port};
        runAdb(removeForward);
    }
}

void AndroidRunnerWorker::onProcessIdChanged(qint64 pid)
{
    // Don't write to m_psProc from a different thread
    QTC_ASSERT(QThread::currentThread() == thread(), return);
    qCDebug(androidRunWorkerLog) << "Process ID changed from:" << m_processPID
                                 << "to:" << pid;
    m_processPID = pid;
    if (pid == -1) {
        emit remoteProcessFinished(QLatin1String("\n\n") + tr("\"%1\" died.")
                                   .arg(m_packageName));
        // App died/killed. Reset log, monitor, jdb & gdbserver/lldb-server processes.
        m_adbLogcatProcess.reset();
        m_psIsAlive.reset();
        m_jdbProcess.reset();
        m_debugServerProcess.reset();

        // Run adb commands after application quit.
        for (const QString &entry: m_afterFinishAdbCommands)
            runAdb(entry.split(' ', Qt::SkipEmptyParts));
    } else {
        // In debugging cases this will be funneled to the engine to actually start
        // and attach gdb. Afterwards this ends up in handleRemoteDebuggerRunning() below.
        emit remoteProcessStarted(m_localDebugServerPort, m_qmlServer, m_processPID);
        logcatReadStandardOutput();
        QTC_ASSERT(!m_psIsAlive, /**/);
        QStringList isAliveArgs = selector() << "shell" << pidPollingScript.arg(m_processPID);
        m_psIsAlive.reset(AndroidManager::runAdbCommandDetached(isAliveArgs));
        QTC_ASSERT(m_psIsAlive, return);
        m_psIsAlive->setObjectName("IsAliveProcess");
        m_psIsAlive->setProcessChannelMode(QProcess::MergedChannels);
        connect(m_psIsAlive.get(), QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                this, bind(&AndroidRunnerWorker::onProcessIdChanged, this, -1));
    }
}

} // namespace Internal
} // namespace Android
