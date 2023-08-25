// Copyright (C) 2018 BogDan Vatra <bog_dan_ro@yahoo.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "androidconfigurations.h"
#include "androidconstants.h"
#include "androidmanager.h"
#include "androidrunnerworker.h"
#include "androidtr.h"

#include <debugger/debuggerkitaspect.h>
#include <debugger/debuggerrunconfigurationaspect.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/environmentaspect.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/runconfigurationaspects.h>
#include <projectexplorer/runcontrol.h>
#include <projectexplorer/target.h>

#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitaspect.h>

#include <utils/async.h>
#include <utils/fileutils.h>
#include <utils/hostosinfo.h>
#include <utils/process.h>
#include <utils/stringutils.h>
#include <utils/temporaryfile.h>
#include <utils/url.h>

#include <QDate>
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

static const QString pidPollingScript = QStringLiteral("while [ -d /proc/%1 ]; do sleep 1; done");
static const QRegularExpression userIdPattern("u(\\d+)_a");

static const int s_jdbTimeout = 5000;

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

static qint64 extractPID(const QString &output, const QString &packageName)
{
    qint64 pid = -1;
    for (const QString &tuple : output.split('\n')) {
        // Make sure to remove null characters which might be present in the provided output
        const QStringList parts = tuple.simplified().remove(QChar('\0')).split(':');
        if (parts.length() == 2 && parts.first() == packageName) {
            pid = parts.last().toLongLong();
            break;
        }
    }
    return pid;
}

static void findProcessPIDAndUser(QPromise<PidUserPair> &promise,
                                  QStringList selector,
                                  const QString &packageName,
                                  bool preNougat)
{
    if (packageName.isEmpty())
        return;

    static const QString pidScript = "pidof -s '%1'";
    static const QString pidScriptPreNougat = QStringLiteral("for p in /proc/[0-9]*; "
                                                    "do cat <$p/cmdline && echo :${p##*/}; done");
    QStringList args = {selector};
    FilePath adbPath = AndroidConfigurations::currentConfig().adbToolPath();
    args.append("shell");
    args.append(preNougat ? pidScriptPreNougat : pidScript.arg(packageName));

    qint64 processPID = -1;
    chrono::high_resolution_clock::time_point start = chrono::high_resolution_clock::now();
    do {
        QThread::msleep(200);
        Process proc;
        proc.setCommand({adbPath, args});
        proc.runBlocking();
        const QString out = proc.allOutput();
        if (preNougat) {
            processPID = extractPID(out, packageName);
        } else {
            if (!out.isEmpty())
                processPID = out.trimmed().toLongLong();
        }
    } while ((processPID == -1 || processPID == 0) && !isTimedOut(start) && !promise.isCanceled());

    qCDebug(androidRunWorkerLog) << "PID found:" << processPID << ", PreNougat:" << preNougat;

    qint64 processUser = 0;
    if (processPID > 0 && !promise.isCanceled()) {
        args = {selector};
        args.append({"shell", "ps", "-o", "user", "-p"});
        args.append(QString::number(processPID));
        Process proc;
        proc.setCommand({adbPath, args});
        proc.runBlocking();
        const QString out = proc.allOutput();
        if (!out.isEmpty()) {
            QRegularExpressionMatch match;
            qsizetype matchPos = out.indexOf(userIdPattern, 0, &match);
            if (matchPos >= 0 && match.capturedLength(1) > 0) {
                bool ok = false;
                processUser = match.captured(1).toInt(&ok);
                if (!ok)
                    processUser = 0;
            }
        }
    }

    qCDebug(androidRunWorkerLog) << "USER found:" << processUser;

    if (!promise.isCanceled())
        promise.addResult(PidUserPair(processPID, processUser));
}

static QString gdbServerArch(const QString &androidAbi)
{
    if (androidAbi == ProjectExplorer::Constants::ANDROID_ABI_ARM64_V8A)
        return QString("arm64");
    if (androidAbi == ProjectExplorer::Constants::ANDROID_ABI_ARMEABI_V7A)
        return QString("arm");
    // That's correct for x86_64 and x86, and best guess at anything that will evolve:
    return androidAbi;
}

static QString lldbServerArch2(const QString &androidAbi)
{
    if (androidAbi == ProjectExplorer::Constants::ANDROID_ABI_ARMEABI_V7A)
        return {"arm"};
    if (androidAbi == ProjectExplorer::Constants::ANDROID_ABI_X86)
        return {"i386"};
    if (androidAbi == ProjectExplorer::Constants::ANDROID_ABI_ARM64_V8A)
        return {"aarch64"};
    // Correct for x86_64 and best guess at anything that will evolve:
    return androidAbi; // x86_64
}

static FilePath debugServer(bool useLldb, const Target *target)
{
    QtSupport::QtVersion *qtVersion = QtSupport::QtKitAspect::qtVersion(target->kit());
    QString preferredAbi = AndroidManager::apkDevicePreferredAbi(target);

    const AndroidConfig &config = AndroidConfigurations::currentConfig();

    if (useLldb) {
        // Search suitable lldb-server binary.
        const FilePath prebuilt = config.ndkLocation(qtVersion) / "toolchains/llvm/prebuilt";
        const QString abiNeedle = lldbServerArch2(preferredAbi);

        // The new, built-in LLDB.
        const QDir::Filters dirFilter = HostOsInfo::isWindowsHost() ? QDir::Files
                                                                    : QDir::Files|QDir::Executable;
        FilePath lldbServer;
        const auto handleLldbServerCandidate = [&abiNeedle, &lldbServer] (const FilePath &path) {
            if (path.parentDir().fileName() == abiNeedle) {
                lldbServer = path;
                return IterationPolicy::Stop;
            }
            return IterationPolicy::Continue;
        };
        prebuilt.iterateDirectory(handleLldbServerCandidate,
                                  {{"lldb-server"}, dirFilter, QDirIterator::Subdirectories});
        if (!lldbServer.isEmpty())
            return lldbServer;
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
{
    auto runControl = runner->runControl();
    m_useLldb = Debugger::DebuggerKitAspect::engineType(runControl->kit())
                    == Debugger::LldbEngineType;
    auto aspect = runControl->aspect<Debugger::DebuggerRunConfigurationAspect>();
    Utils::Id runMode = runControl->runMode();
    const bool debuggingMode = runMode == ProjectExplorer::Constants::DEBUG_RUN_MODE;
    m_useCppDebugger = debuggingMode && aspect->useCppDebugger;
    if (debuggingMode && aspect->useQmlDebugger)
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
        const bool isListening = server.listen(QHostAddress::LocalHost);
        QTC_ASSERT(isListening,
                   qDebug() << Tr::tr("No free ports available on host for QML debugging."));
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

    m_extraEnvVars = runControl->aspect<EnvironmentAspect>()->environment;
    qCDebug(androidRunWorkerLog).noquote() << "Environment variables for the app"
                                           << m_extraEnvVars.toStringList();

    if (target->buildConfigurations().first()->buildType() != BuildConfiguration::BuildType::Release)
        m_extraAppParams = runControl->commandLine().arguments();

    if (const Store sd = runControl->settingsData(Constants::ANDROID_AM_START_ARGS);
        !sd.values().isEmpty()) {
        QTC_CHECK(sd.first().type() == QVariant::String);
        const QString startArgs = sd.first().toString();
        m_amStartExtraArgs = ProcessArgs::splitArgs(startArgs, OsTypeOtherUnix);
    }

    if (const Store sd = runControl->settingsData(Constants::ANDROID_PRESTARTSHELLCMDLIST);
        !sd.values().isEmpty()) {
        QTC_CHECK(sd.first().type() == QVariant::String);
        const QStringList commands = sd.first().toString().split('\n', Qt::SkipEmptyParts);
        for (const QString &shellCmd : commands)
            m_beforeStartAdbCommands.append(QString("shell %1").arg(shellCmd));
    }

    if (const Store sd = runControl->settingsData(Constants::ANDROID_POSTFINISHSHELLCMDLIST);
        !sd.values().isEmpty()) {
        QTC_CHECK(sd.first().type() == QVariant::String);
        const QStringList commands = sd.first().toString().split('\n', Qt::SkipEmptyParts);
        for (const QString &shellCmd : commands)
            m_afterFinishAdbCommands.append(QString("shell %1").arg(shellCmd));
    }

    m_debugServerPath = debugServer(m_useLldb, target);
    qCDebug(androidRunWorkerLog).noquote() << "Device Serial:" << m_deviceSerialNumber
                                           << ", API level:" << m_apiLevel
                                           << ", Extra Start Args:" << m_amStartExtraArgs
                                           << ", Before Start ADB cmds:" << m_beforeStartAdbCommands
                                           << ", After finish ADB cmds:" << m_afterFinishAdbCommands
                                           << ", Debug server path:" << m_debugServerPath;

    QtSupport::QtVersion *version = QtSupport::QtKitAspect::qtVersion(target->kit());
    m_useAppParamsForQmlDebugger = version->qtVersion() >= QVersionNumber(5, 12);
}

AndroidRunnerWorker::~AndroidRunnerWorker()
{
    if (m_processPID != -1)
        forceStop();

    if (!m_pidFinder.isFinished())
        m_pidFinder.cancel();
}

bool AndroidRunnerWorker::runAdb(const QStringList &args, QString *stdOut,
                                 QString *stdErr, const QByteArray &writeData)
{
    QStringList adbArgs = selector() + args;
    SdkToolResult result = AndroidManager::runAdbCommand(adbArgs, writeData);
    if (!result.success())
        emit remoteErrorOutput(result.stdErr());
    if (stdOut)
        *stdOut = result.stdOut();
    if (stdErr)
        *stdErr = result.stdErr();
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
    const QScopeGuard cleanup([this, tempDebugServerPath] {
        if (!runAdb({"shell", "rm", "-f", tempDebugServerPath}))
            qCDebug(androidRunWorkerLog) << "Debug server cleanup failed.";
    });

    // Copy gdbserver to temp location
    if (!runAdb({"push", m_debugServerPath.toString(), tempDebugServerPath})) {
        qCDebug(androidRunWorkerLog) << "Debug server upload to temp directory failed";
        return false;
    }

    QStringList adbArgs = {"shell", "run-as", m_packageName};
    if (m_processUser > 0)
        adbArgs << "--user" << QString::number(m_processUser);
    // Copy gdbserver from temp location to app directory
    if (!runAdb(adbArgs + QStringList({"cp" , tempDebugServerPath, debugServerFileName}))) {
        qCDebug(androidRunWorkerLog) << "Debug server copy from temp directory failed";
        return false;
    }

    const bool ok = runAdb(adbArgs + QStringList({"chmod", "777", debugServerFileName}));
    QTC_ASSERT(ok, qCDebug(androidRunWorkerLog) << "Debug server chmod 777 failed.");
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
    QStringList adbArgs = {"shell", "run-as", m_packageName};
    if (m_processUser > 0)
        adbArgs << "--user" << QString::number(m_processUser);
    const bool success = runAdb(adbArgs + QStringList({"ls", filePath, "2>/dev/null"}),
                                &output);
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
        logcatProcess(m_adbLogcatProcess->readAllRawStandardError(), m_stderrBuffer, true);
}

void AndroidRunnerWorker::logcatReadStandardOutput()
{
    if (m_processPID != -1)
        logcatProcess(m_adbLogcatProcess->readAllRawStandardOutput(), m_stdoutBuffer, false);
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
    for (const QByteArray &msg : std::as_const(lines)) {
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

void Android::Internal::AndroidRunnerWorker::asyncStartLogcat()
{
    // Its assumed that the device or avd returned by selector() is online.
    // Start the logcat process before app starts.
    QTC_CHECK(!m_adbLogcatProcess);

    // Ideally AndroidManager::runAdbCommandDetached() should be used, but here
    // we need to connect the readyRead signals from logcat otherwise we might
    // lost some output between the process start and connecting those signals.
    m_adbLogcatProcess.reset(new Process);

    connect(m_adbLogcatProcess.get(), &Process::readyReadStandardOutput,
            this, &AndroidRunnerWorker::logcatReadStandardOutput);
    connect(m_adbLogcatProcess.get(), &Process::readyReadStandardError,
            this, &AndroidRunnerWorker::logcatReadStandardError);

    // Get target current time to fetch only recent logs
    QString dateInSeconds;
    QStringList timeArg;
    if (runAdb({"shell", "date", "+%s"}, &dateInSeconds)) {
        timeArg << "-T";
        timeArg << QDateTime::fromSecsSinceEpoch(dateInSeconds.toInt())
                       .toString("MM-dd hh:mm:ss.mmm");
    }

    const QStringList logcatArgs = selector() << "logcat" << timeArg;
    const FilePath adb = AndroidConfigurations::currentConfig().adbToolPath();
    qCDebug(androidRunWorkerLog).noquote() << "Running logcat command (async):"
                                           << CommandLine(adb, logcatArgs).toUserOutput();
    m_adbLogcatProcess->setCommand({adb, logcatArgs});
    m_adbLogcatProcess->start();
    if (m_adbLogcatProcess->waitForStarted(500) && m_adbLogcatProcess->state() == QProcess::Running)
        m_adbLogcatProcess->setObjectName("AdbLogcatProcess");
}

void AndroidRunnerWorker::asyncStartHelper()
{
    forceStop();
    asyncStartLogcat();

    for (const QString &entry : std::as_const(m_beforeStartAdbCommands))
        runAdb(entry.split(' ', Qt::SkipEmptyParts));

    QStringList args({"shell", "am", "start"});
    args << "-n" << m_intentName;
    if (m_useCppDebugger)
        args << "-D";

    if (m_qmlDebugServices != QmlDebug::NoQmlDebugServices) {
        // currently forward to same port on device and host
        const QString port = QString("tcp:%1").arg(m_qmlServer.port());
        QStringList removeForward{{"forward", "--remove", port}};
        removeForwardPort(port);
        if (!runAdb({"forward", port, port})) {
            emit remoteProcessFinished(Tr::tr("Failed to forward QML debugging ports."));
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

    args << m_amStartExtraArgs;

    if (!m_extraAppParams.isEmpty()) {
        QStringList appArgs =
                Utils::ProcessArgs::splitArgs(m_extraAppParams, Utils::OsType::OsTypeLinux);
        qCDebug(androidRunWorkerLog).noquote() << "Using application arguments: " << appArgs;
        args << "-e" << "extraappparams"
             << QString::fromLatin1(appArgs.join(' ').toUtf8().toBase64());
    }

    if (m_extraEnvVars.hasChanges()) {
        args << "-e" << "extraenvvars"
             << QString::fromLatin1(m_extraEnvVars.toStringList().join('\t')
                                    .toUtf8().toBase64());
    }

    QString stdErr;
    const bool startResult = runAdb(args, nullptr, &stdErr);
    if (!startResult) {
        emit remoteProcessFinished(Tr::tr("Failed to start the activity."));
        return;
    }

    if (!stdErr.isEmpty()) {
        emit remoteErrorOutput(Tr::tr("Activity Manager threw the error: %1").arg(stdErr));
        return;
    }
}

void AndroidRunnerWorker::startNativeDebugging()
{
    // run-as <package-name> pwd fails on API 22 so route the pwd through shell.
    QString packageDir;
    QStringList adbArgs = {"shell", "run-as", m_packageName};
    if (m_processUser > 0)
        adbArgs << "--user" << QString::number(m_processUser);
    if (!runAdb(adbArgs + QStringList({"/system/bin/sh", "-c", "pwd"}),
                &packageDir)) {
        emit remoteProcessFinished(Tr::tr("Failed to find application directory."));
        return;
    }
    // Add executable flag to package dir. Gdb can't connect to running server on device on
    // e.g. on Android 8 with NDK 10e
    runAdb(adbArgs + QStringList({"chmod", "a+x", packageDir.trimmed()}));
    if (!m_debugServerPath.exists()) {
        QString msg = Tr::tr("Cannot find C++ debug server in NDK installation.");
        if (m_useLldb)
            msg += "\n" + Tr::tr("The lldb-server binary has not been found.");
        emit remoteProcessFinished(msg);
        return;
    }

    QString debugServerFile;
    if (m_useLldb) {
        debugServerFile = "./lldb-server";
        runAdb(adbArgs + QStringList({"killall", "lldb-server"}));
        if (!uploadDebugServer(debugServerFile)) {
            emit remoteProcessFinished(Tr::tr("Cannot copy C++ debug server."));
            return;
        }
    } else {
        if (packageFileExists("./lib/gdbserver")) {
            debugServerFile = "./lib/gdbserver";
            qCDebug(androidRunWorkerLog) << "Found GDB server " + debugServerFile;
            runAdb(adbArgs + QStringList({"killall", "gdbserver"}));
        } else if (packageFileExists("./lib/libgdbserver.so")) {
            debugServerFile = "./lib/libgdbserver.so";
            qCDebug(androidRunWorkerLog) << "Found GDB server " + debugServerFile;
            runAdb(adbArgs + QStringList({"killall", "libgdbserver.so"}));
        } else {
            // Armv8. symlink lib is not available.
            debugServerFile = "./gdbserver";
            // Kill the previous instances of gdbserver. Do this before copying the gdbserver.
            runAdb(adbArgs + QStringList({"killall", "gdbserver"}));
            if (!uploadDebugServer("./gdbserver")) {
                emit remoteProcessFinished(Tr::tr("Cannot copy C++ debug server."));
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

bool AndroidRunnerWorker::startDebuggerServer(const QString &packageDir,
                                              const QString &debugServerFile,
                                              QString *errorStr)
{
    QStringList adbArgs = {"shell", "run-as", m_packageName};
    if (m_processUser > 0)
        adbArgs << "--user" << QString::number(m_processUser);
    if (m_useLldb) {
        QString lldbServerErr;
        QStringList lldbServerArgs = selector();
        lldbServerArgs += adbArgs;
        lldbServerArgs << debugServerFile
                       << "platform"
                       // << "--server"  // Can lead to zombie servers
                       << "--listen" << QString("*:%1").arg(m_localDebugServerPort.toString());
        m_debugServerProcess.reset(AndroidManager::startAdbProcess(lldbServerArgs, &lldbServerErr));

        if (!m_debugServerProcess) {
            qCDebug(androidRunWorkerLog) << "Debugger process failed to start" << lldbServerErr;
            if (errorStr)
                *errorStr = Tr::tr("Failed to start debugger server.");
            return false;
        }
        qCDebug(androidRunWorkerLog) << "Debugger process started";
        m_debugServerProcess->setObjectName("AndroidDebugServerProcess");

    } else {
        QString gdbServerSocket = packageDir + "/debug-socket";
        runAdb(adbArgs + QStringList({"rm", gdbServerSocket}));

        QString gdbProcessErr;
        QStringList gdbServerErr = selector();
        gdbServerErr += adbArgs;
        gdbServerErr << debugServerFile
                     << "--multi" << "+" + gdbServerSocket;
        m_debugServerProcess.reset(AndroidManager::startAdbProcess(gdbServerErr, &gdbProcessErr));

        if (!m_debugServerProcess) {
            qCDebug(androidRunWorkerLog) << "Debugger process failed to start" << gdbServerErr;
            if (errorStr)
                *errorStr = Tr::tr("Failed to start debugger server.");
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
                *errorStr = Tr::tr("Failed to forward C++ debugging ports.");
            return false;
        }
        m_afterFinishAdbCommands.push_back(removeForward.join(' '));
    }
    return true;
}

void AndroidRunnerWorker::asyncStart()
{
    asyncStartHelper();

    m_pidFinder = Utils::onResultReady(Utils::asyncRun(findProcessPIDAndUser,
                                                       selector(),
                                                       m_packageName,
                                                       m_isPreNougat),
                                       this,
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
        emit remoteProcessFinished(Tr::tr("Failed to forward JDB debugging ports."));
        return;
    }
    m_afterFinishAdbCommands.push_back(removeForward.join(' '));

    const FilePath jdbPath = AndroidConfigurations::currentConfig().openJDKLocation()
            .pathAppended("bin/jdb").withExecutableSuffix();

    QStringList jdbArgs("-connect");
    jdbArgs << QString("com.sun.jdi.SocketAttach:hostname=localhost,port=%1")
               .arg(m_localJdbServerPort.toString());
    qCDebug(androidRunWorkerLog).noquote()
            << "Starting JDB:" << CommandLine(jdbPath, jdbArgs).toUserOutput();
    m_jdbProcess.reset(new Process);
    m_jdbProcess->setProcessChannelMode(QProcess::MergedChannels);
    m_jdbProcess->setCommand({jdbPath, jdbArgs});
    m_jdbProcess->setReaperTimeout(s_jdbTimeout);
    m_jdbProcess->start();
    if (!m_jdbProcess->waitForStarted()) {
        emit remoteProcessFinished(Tr::tr("Failed to start JDB."));
        m_jdbProcess.reset();
        return;
    }
    m_jdbProcess->setObjectName("JdbProcess");
}

void AndroidRunnerWorker::handleJdbSettled()
{
    qCDebug(androidRunWorkerLog) << "Handle JDB settled";
    auto waitForCommand = [this] {
        for (int i = 0; i < 120 && m_jdbProcess->state() == QProcess::Running; ++i) {
            m_jdbProcess->waitForReadyRead(500);
            const QByteArray lines = m_jdbProcess->readAllRawStandardOutput();
            const auto linesList = lines.split('\n');
            for (const auto &line : linesList) {
                auto msg = line.trimmed();
                if (msg.startsWith(">"))
                    return true;
            }
        }
        return false;
    };

    const QStringList commands{"threads", "cont", "exit"};

    for (const QString &command : commands) {
        if (waitForCommand())
            m_jdbProcess->write(QString("%1\n").arg(command));
    }

    if (!m_jdbProcess->waitForFinished(s_jdbTimeout)) {
        m_jdbProcess.reset();
    } else if (m_jdbProcess->exitStatus() == QProcess::NormalExit && m_jdbProcess->exitCode() == 0) {
        qCDebug(androidRunWorkerLog) << "JDB settled";
        return;
    }

    emit remoteProcessFinished(Tr::tr("Cannot attach JDB to the running application."));
}

void AndroidRunnerWorker::removeForwardPort(const QString &port)
{
    bool found = false;
    SdkToolResult result = AndroidManager::runAdbCommand({"forward", "--list"});

    QString string = result.stdOut();
    const auto lines = string.split('\n');
    for (const QString &line : lines) {
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

void AndroidRunnerWorker::onProcessIdChanged(PidUserPair pidUser)
{
    qint64 pid = pidUser.first;
    qint64 user = pidUser.second;
    // Don't write to m_psProc from a different thread
    QTC_ASSERT(QThread::currentThread() == thread(), return);
    qCDebug(androidRunWorkerLog) << "Process ID changed from:" << m_processPID
                                 << "to:" << pid;
    m_processPID = pid;
    m_processUser = user;
    if (pid == -1) {
        emit remoteProcessFinished(QLatin1String("\n\n") + Tr::tr("\"%1\" died.")
                                   .arg(m_packageName));
        // App died/killed. Reset log, monitor, jdb & gdbserver/lldb-server processes.
        m_adbLogcatProcess.reset();
        m_psIsAlive.reset();
        m_jdbProcess.reset();
        m_debugServerProcess.reset();

        // Run adb commands after application quit.
        for (const QString &entry: std::as_const(m_afterFinishAdbCommands))
            runAdb(entry.split(' ', Qt::SkipEmptyParts));
    } else {
        if (m_useCppDebugger)
            startNativeDebugging();
        // In debugging cases this will be funneled to the engine to actually start
        // and attach gdb. Afterwards this ends up in handleRemoteDebuggerRunning() below.
        emit remoteProcessStarted(m_localDebugServerPort, m_qmlServer, m_processPID);
        logcatReadStandardOutput();
        QTC_ASSERT(!m_psIsAlive, /**/);
        QStringList isAliveArgs = selector() << "shell" << pidPollingScript.arg(m_processPID);
        m_psIsAlive.reset(AndroidManager::startAdbProcess(isAliveArgs));
        QTC_ASSERT(m_psIsAlive, return);
        m_psIsAlive->setObjectName("IsAliveProcess");
        m_psIsAlive->setProcessChannelMode(QProcess::MergedChannels);
        connect(m_psIsAlive.get(), &Process::done, this, [this] { onProcessIdChanged({-1, -1}); });
    }
}

} // namespace Internal
} // namespace Android
