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

#include <solutions/tasking/conditional.h>

#include <utils/hostosinfo.h>
#include <utils/qtcprocess.h>
#include <utils/url.h>

#include <QDate>
#include <QLoggingCategory>
#include <QRegularExpression>
#include <QScopeGuard>
#include <QTcpServer>

#include <chrono>

namespace {
static Q_LOGGING_CATEGORY(androidRunWorkerLog, "qtc.android.run.androidrunnerworker", QtWarningMsg)
static const int GdbTempFileMaxCounter = 20;
}

using namespace ProjectExplorer;
using namespace Tasking;
using namespace Utils;

using namespace std;
using namespace std::chrono_literals;
using namespace std::placeholders;

namespace Android::Internal {

static const QString pidPollingScript = QStringLiteral("while [ -d /proc/%1 ]; do sleep 1; done");
static const QRegularExpression userIdPattern("u(\\d+)_a");

static const std::chrono::milliseconds s_jdbTimeout = 5s;

static const Port s_localJdbServerPort(5038);
static const Port s_localDebugServerPort(5039); // Local end of forwarded debug socket.

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

    if (useLldb) {
        // Search suitable lldb-server binary.
        const FilePath prebuilt = AndroidConfig::ndkLocation(qtVersion) / "toolchains/llvm/prebuilt";
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
        const FilePath path = AndroidConfig::ndkLocation(qtVersion)
                .pathAppended(QString("prebuilt/android-%1/gdbserver/gdbserver")
                              .arg(gdbServerArch(preferredAbi)));
        if (path.exists())
            return path;
    }
    return {};
}

AndroidRunnerWorker::AndroidRunnerWorker(RunWorker *runner)
{
    auto runControl = runner->runControl();
    m_useLldb = Debugger::DebuggerKitAspect::engineType(runControl->kit())
                    == Debugger::LldbEngineType;
    auto aspect = runControl->aspectData<Debugger::DebuggerRunConfigurationAspect>();
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

    auto target = runControl->target();
    m_packageName = AndroidManager::packageName(target);
    m_intentName = m_packageName + '/' + AndroidManager::activityName(target);
    m_deviceSerialNumber = AndroidManager::deviceSerialNumber(target);
    m_apiLevel = AndroidManager::deviceApiLevel(target);
    qCDebug(androidRunWorkerLog) << "Intent name:" << m_intentName
                                 << "Package name:" << m_packageName;
    qCDebug(androidRunWorkerLog) << "Device API:" << m_apiLevel;

    m_extraEnvVars = runControl->aspectData<EnvironmentAspect>()->environment;
    qCDebug(androidRunWorkerLog).noquote() << "Environment variables for the app"
                                           << m_extraEnvVars.toStringList();

    if (target->buildConfigurations().first()->buildType() != BuildConfiguration::BuildType::Release)
        m_extraAppParams = runControl->commandLine().arguments();

    if (const Store sd = runControl->settingsData(Constants::ANDROID_AM_START_ARGS);
        !sd.isEmpty()) {
        QTC_CHECK(sd.first().typeId() == QMetaType::QString);
        const QString startArgs = sd.first().toString();
        m_amStartExtraArgs = ProcessArgs::splitArgs(startArgs, OsTypeOtherUnix);
    }

    if (const Store sd = runControl->settingsData(Constants::ANDROID_PRESTARTSHELLCMDLIST);
        !sd.isEmpty()) {
        const QVariant &first = sd.first();
        QTC_CHECK(first.typeId() == QMetaType::QStringList);
        const QStringList commands = first.toStringList();
        for (const QString &shellCmd : commands)
            m_beforeStartAdbCommands.append(QString("shell %1").arg(shellCmd));
    }

    if (const Store sd = runControl->settingsData(Constants::ANDROID_POSTFINISHSHELLCMDLIST);
        !sd.isEmpty()) {
        const QVariant &first = sd.first();
        QTC_CHECK(first.typeId() == QMetaType::QStringList);
        const QStringList commands = first.toStringList();
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
    m_taskTreeRunner.setParent(this); // Move m_taskTreeRunner object together with *this into a separate thread.
}

AndroidRunnerWorker::~AndroidRunnerWorker()
{
    if (m_processPID != -1)
        forceStop();
}

bool AndroidRunnerWorker::runAdb(const QStringList &args, QString *stdOut, QString *stdErr)
{
    const SdkToolResult result = AndroidManager::runAdbCommand(selector() + args);
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
    const QString pidString = QString::number(m_processPID);
    if (m_processPID != -1 && output == pidString
        && !runAdb({"shell", "run-as", m_packageName, "kill", "-9", pidString})) {
        runAdb({"shell", "kill", "-9", pidString});
    }
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

void AndroidRunnerWorker::asyncStartLogcat()
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
    const FilePath adb = AndroidConfig::adbToolPath();
    qCDebug(androidRunWorkerLog).noquote() << "Running logcat command (async):"
                                           << CommandLine(adb, logcatArgs).toUserOutput();
    m_adbLogcatProcess->setCommand({adb, logcatArgs});
    m_adbLogcatProcess->start();
    if (m_adbLogcatProcess->waitForStarted(500ms) && m_adbLogcatProcess->state() == QProcess::Running)
        m_adbLogcatProcess->setObjectName("AdbLogcatProcess");
}

void AndroidRunnerWorker::asyncStartHelper()
{
    forceStop();
    asyncStartLogcat();
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
    startDebuggerServer(packageDir, debugServerFile);
}

void AndroidRunnerWorker::startDebuggerServer(const QString &packageDir,
                                              const QString &debugServerFile)
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
                       << "--listen" << QString("*:%1").arg(s_localDebugServerPort.toString());
        m_debugServerProcess.reset(AndroidManager::startAdbProcess(lldbServerArgs, &lldbServerErr));

        if (!m_debugServerProcess) {
            qCDebug(androidRunWorkerLog) << "Debugger process failed to start" << lldbServerErr;
            emit remoteProcessFinished(Tr::tr("Failed to start debugger server."));
            return;
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
            emit remoteProcessFinished(Tr::tr("Failed to start debugger server."));
            return;
        }
        qCDebug(androidRunWorkerLog) << "Debugger process started";
        m_debugServerProcess->setObjectName("AndroidDebugServerProcess");

        removeForwardPort("tcp:" + s_localDebugServerPort.toString(),
                          "localfilesystem:" + gdbServerSocket, "C++");
    }
}

ExecutableItem AndroidRunnerWorker::removeForwardPortRecipe(
    const QString &port, const QString &adbArg, const QString &portType)
{
    const auto onForwardListSetup = [](Process &process) {
        process.setCommand({AndroidConfig::adbToolPath(), {"forward", "--list"}});
    };
    const auto onForwardListDone = [port](const Process &process) {
        return process.cleanedStdOut().trimmed().contains(port);
    };

    const auto onForwardRemoveSetup = [this, port](Process &process) {
        process.setCommand({AndroidConfig::adbToolPath(), {selector(), "--remove", port}});
    };
    const auto onForwardRemoveDone = [this](const Process &process) {
        emit remoteErrorOutput(process.cleanedStdErr().trimmed());
        return true;
    };

    const auto onForwardPortSetup = [this, port, adbArg](Process &process) {
        process.setCommand({AndroidConfig::adbToolPath(), {selector(), "forward", port, adbArg}});
    };
    const auto onForwardPortDone = [this, port, portType](DoneWith result) {
        if (result == DoneWith::Success)
            m_afterFinishAdbCommands.push_back("forward --remove " + port);
        else
            emit remoteProcessFinished(Tr::tr("Failed to forward %1 debugging ports.").arg(portType));
    };

    return Group {
        If (ProcessTask(onForwardListSetup, onForwardListDone)) >> Then {
            ProcessTask(onForwardRemoveSetup, onForwardRemoveDone, CallDoneIf::Error)
        },
        ProcessTask(onForwardPortSetup, onForwardPortDone)
    };
}

ExecutableItem AndroidRunnerWorker::preStartRecipe()
{
    const QString port = "tcp:" + QString::number(m_qmlServer.port());

    const Storage<QStringList> argsStorage;
    const LoopList iterator(m_beforeStartAdbCommands);

    const auto onArgsSetup = [this, argsStorage] {
        *argsStorage = {"shell", "am", "start", "-n", m_intentName};
        if (m_useCppDebugger)
            *argsStorage << "-D";
    };

    const auto onPreCommandSetup = [this, iterator](Process &process) {
        process.setCommand({AndroidConfig::adbToolPath(),
                            {selector(), iterator->split(' ', Qt::SkipEmptyParts)}});
    };
    const auto onPreCommandDone = [this](const Process &process) {
        emit remoteErrorOutput(process.cleanedStdErr().trimmed());
    };

    const auto onQmlDebugSetup = [this] {
        return m_qmlDebugServices == QmlDebug::NoQmlDebugServices ? SetupResult::StopWithSuccess
                                                                  : SetupResult::Continue;
    };
    const auto onQmlDebugDone = [this, argsStorage] {
        const QString qmljsdebugger = QString("port:%1,block,services:%2")
            .arg(m_qmlServer.port()).arg(QmlDebug::qmlDebugServices(m_qmlDebugServices));

        if (m_useAppParamsForQmlDebugger) {
            if (!m_extraAppParams.isEmpty())
                m_extraAppParams.prepend(' ');
            m_extraAppParams.prepend("-qmljsdebugger=" + qmljsdebugger);
        } else {
            *argsStorage << "-e" << "qml_debug" << "true"
                         << "-e" << "qmljsdebugger" << qmljsdebugger;
        }
    };

    const auto onActivitySetup = [this, argsStorage](Process &process) {
        QStringList args = *argsStorage;
        args << m_amStartExtraArgs;

        if (!m_extraAppParams.isEmpty()) {
            const QStringList appArgs =
                ProcessArgs::splitArgs(m_extraAppParams, Utils::OsType::OsTypeLinux);
            qCDebug(androidRunWorkerLog).noquote() << "Using application arguments: " << appArgs;
            args << "-e" << "extraappparams"
                 << QString::fromLatin1(appArgs.join(' ').toUtf8().toBase64());
        }

        if (m_extraEnvVars.hasChanges()) {
            args << "-e" << "extraenvvars"
                 << QString::fromLatin1(m_extraEnvVars.toStringList().join('\t')
                                            .toUtf8().toBase64());
        }
        process.setCommand({AndroidConfig::adbToolPath(), {selector(), args}});
    };
    const auto onActivityDone = [this](const Process &process) {
        emit remoteProcessFinished(Tr::tr("Activity Manager error: %1")
                                       .arg(process.cleanedStdErr().trimmed()));
    };

    return Group {
        argsStorage,
        onGroupSetup(onArgsSetup),
        For {
            iterator,
            ProcessTask(onPreCommandSetup, onPreCommandDone, CallDoneIf::Error)
        },
        Group {
            onGroupSetup(onQmlDebugSetup),
            removeForwardPortRecipe(port, port, "QML"),
            onGroupDone(onQmlDebugDone, CallDoneIf::Success)
        },
        ProcessTask(onActivitySetup, onActivityDone, CallDoneIf::Error)
    };
}

ExecutableItem AndroidRunnerWorker::pidRecipe()
{
    const Storage<PidUserPair> pidStorage;

    const FilePath adbPath = AndroidConfig::adbToolPath();
    const QStringList args = selector();
    const QString pidScript = isPreNougat()
        ? QString("for p in /proc/[0-9]*; do cat <$p/cmdline && echo :${p##*/}; done")
        : QString("pidof -s '%1'").arg(m_packageName);

    const auto onPidSetup = [adbPath, args, pidScript](Process &process) {
        process.setCommand({adbPath, {args, "shell", pidScript}});
    };
    const auto onPidDone = [pidStorage, packageName = m_packageName,
                            isPreNougat = isPreNougat()](const Process &process) {
        const QString out = process.allOutput();
        if (isPreNougat)
            pidStorage->first = extractPID(out, packageName);
        else if (!out.isEmpty())
            pidStorage->first = out.trimmed().toLongLong();
    };

    const auto onUserSetup = [pidStorage, adbPath, args](Process &process) {
        process.setCommand({adbPath, {args, "shell", "ps", "-o", "user", "-p",
                                      QString::number(pidStorage->first)}});
    };
    const auto onUserDone = [pidStorage](const Process &process) {
        const QString out = process.allOutput();
        if (out.isEmpty())
            return DoneResult::Error;

        QRegularExpressionMatch match;
        qsizetype matchPos = out.indexOf(userIdPattern, 0, &match);
        if (matchPos >= 0 && match.capturedLength(1) > 0) {
            bool ok = false;
            const qint64 processUser = match.captured(1).toInt(&ok);
            if (ok) {
                pidStorage->second = processUser;
                return DoneResult::Success;
            }
        }
        return DoneResult::Error;
    };

    return Group {
        pidStorage,
        onGroupSetup([pidStorage] { *pidStorage = {-1, 0}; }),
        Forever {
            stopOnSuccess,
            ProcessTask(onPidSetup, onPidDone, CallDoneIf::Success),
            TimeoutTask([](std::chrono::milliseconds &timeout) { timeout = 200ms; },
                        DoneResult::Error)
        }.withTimeout(45s),
        ProcessTask(onUserSetup, onUserDone, CallDoneIf::Success),
        onGroupDone([pidStorage, this] { onProcessIdChanged(*pidStorage); })
    };
}

void AndroidRunnerWorker::asyncStart()
{
    asyncStartHelper();

    const Group recipe {
        preStartRecipe(),
        pidRecipe()
    };

    m_taskTreeRunner.start(recipe);
}

void AndroidRunnerWorker::asyncStop()
{
    m_taskTreeRunner.reset();
    if (m_processPID != -1)
        forceStop();

    m_jdbProcess.reset();
    m_debugServerProcess.reset();
}

void AndroidRunnerWorker::handleJdbWaiting()
{
    if (!removeForwardPort("tcp:" + s_localJdbServerPort.toString(),
                           "jdwp:" + QString::number(m_processPID), "JDB"))
        return;

    const FilePath jdbPath = AndroidConfig::openJDKLocation()
            .pathAppended("bin/jdb").withExecutableSuffix();

    QStringList jdbArgs("-connect");
    jdbArgs << QString("com.sun.jdi.SocketAttach:hostname=localhost,port=%1")
               .arg(s_localJdbServerPort.toString());
    qCDebug(androidRunWorkerLog).noquote()
            << "Starting JDB:" << CommandLine(jdbPath, jdbArgs).toUserOutput();
    m_jdbProcess.reset(new Process);
    m_jdbProcess->setProcessChannelMode(QProcess::MergedChannels);
    m_jdbProcess->setCommand({jdbPath, jdbArgs});
    m_jdbProcess->setReaperTimeout(s_jdbTimeout);
    m_jdbProcess->setProcessMode(ProcessMode::Writer);
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
            m_jdbProcess->waitForReadyRead(500ms);
            const QByteArray lines = m_jdbProcess->readAllRawStandardOutput();
            qCDebug(androidRunWorkerLog) << "JDB output:" << lines;
            const auto linesList = lines.split('\n');
            for (const auto &line : linesList) {
                auto msg = line.trimmed();
                if (msg.startsWith(">"))
                    return true;
            }
        }
        return false;
    };

    const QStringList commands{"ignore uncaught java.lang.Throwable", "threads", "cont", "exit"};

    for (const QString &command : commands) {
        if (waitForCommand()) {
            qCDebug(androidRunWorkerLog) << "JDB input:" << command;
            m_jdbProcess->write(QString("%1\n").arg(command));
        }
    }

    if (!m_jdbProcess->waitForFinished(s_jdbTimeout)) {
        m_jdbProcess.reset();
    } else if (m_jdbProcess->exitStatus() == QProcess::NormalExit && m_jdbProcess->exitCode() == 0) {
        qCDebug(androidRunWorkerLog) << "JDB settled";
        return;
    }

    emit remoteProcessFinished(Tr::tr("Cannot attach JDB to the running application."));
}

bool AndroidRunnerWorker::removeForwardPort(const QString &port, const QString &adbArg,
                                            const QString &portType)
{
    const SdkToolResult result = AndroidManager::runAdbCommand({"forward", "--list"});
    if (result.stdOut().contains(port))
        runAdb({"forward", "--remove", port});
    if (runAdb({"forward", port, adbArg})) {
        m_afterFinishAdbCommands.push_back("forward --remove " + port);
        return true;
    }
    emit remoteProcessFinished(Tr::tr("Failed to forward %1 debugging ports.").arg(portType));
    return false;
}

void AndroidRunnerWorker::onProcessIdChanged(const PidUserPair &pidUser)
{
    qCDebug(androidRunWorkerLog) << "Process ID changed from:" << m_processPID
                                 << "to:" << pidUser.first;
    m_processPID = pidUser.first;
    m_processUser = pidUser.second;
    if (m_processPID == -1) {
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
        emit remoteProcessStarted(s_localDebugServerPort, m_qmlServer, m_processPID);
        logcatReadStandardOutput();
        QTC_ASSERT(!m_psIsAlive, /**/);
        QStringList isAliveArgs = selector() << "shell" << pidPollingScript.arg(m_processPID);
        m_psIsAlive.reset(AndroidManager::startAdbProcess(isAliveArgs));
        QTC_ASSERT(m_psIsAlive, return);
        m_psIsAlive->setObjectName("IsAliveProcess");
        m_psIsAlive->setProcessChannelMode(QProcess::MergedChannels);
        connect(m_psIsAlive.get(), &Process::done, this, [this] {
            m_psIsAlive.release()->deleteLater();
            onProcessIdChanged({-1, -1});
        });
    }
}

} // namespace Android::Internal
