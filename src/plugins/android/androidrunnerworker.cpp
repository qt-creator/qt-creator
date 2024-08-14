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

#include <solutions/tasking/barrier.h>
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

static const std::chrono::milliseconds s_jdbTimeout = 60s;

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

void AndroidRunnerWorker::setAndroidDeviceInfo(const AndroidDeviceInfo &info)
{
    m_deviceSerialNumber = info.serialNumber;
    m_apiLevel = info.sdk;
    qCDebug(androidRunWorkerLog) << "Android Device Info changed"
                                 << m_deviceSerialNumber << m_apiLevel;
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
    const QString gdbServerSocket = packageDir + "/debug-socket";

    QStringList adbArgs = {"shell", "run-as", m_packageName};
    if (m_processUser > 0)
        adbArgs << "--user" << QString::number(m_processUser);

    if (!m_useLldb)
        runAdb(adbArgs + QStringList({"rm", gdbServerSocket}));

    QStringList serverArgs = selector() + adbArgs + QStringList{debugServerFile};

    if (m_useLldb)
        serverArgs += QStringList{"platform", "--listen", "*:" + s_localDebugServerPort.toString()};
    else
        serverArgs += QStringList{"--multi", "+" + gdbServerSocket};

    QString serverError;
    m_debugServerProcess.reset(AndroidManager::startAdbProcess(serverArgs, &serverError));
    if (!m_debugServerProcess) {
        qCDebug(androidRunWorkerLog) << "Debugger process failed to start" << serverError;
        emit remoteProcessFinished(Tr::tr("Failed to start debugger server."));
        return;
    }
    qCDebug(androidRunWorkerLog) << "Debugger process started";
    m_debugServerProcess->setObjectName("AndroidDebugServerProcess");

    if (m_useLldb)
        return;

    removeForwardPort("tcp:" + s_localDebugServerPort.toString(),
                      "localfilesystem:" + gdbServerSocket, "C++");
}

CommandLine AndroidRunnerWorker::adbCommand(std::initializer_list<CommandLine::ArgRef> args) const
{
    CommandLine cmd{AndroidConfig::adbToolPath(), args};
    cmd.prependArgs(selector());
    return cmd;
}

QStringList AndroidRunnerWorker::userArgs() const
{
    return m_processUser > 0 ? QStringList{"--user", QString::number(m_processUser)} : QStringList{};
}

QStringList AndroidRunnerWorker::packageArgs() const
{
    // run-as <package-name> pwd fails on API 22 so route the pwd through shell.
    return QStringList{"shell", "run-as", m_packageName} + userArgs();
}

ExecutableItem AndroidRunnerWorker::forceStopRecipe()
{
    const auto onForceStopSetup = [this](Process &process) {
        process.setCommand(adbCommand({"shell", "am", "force-stop", m_packageName}));
    };

    const auto pidCheckSync = Sync([this] { return m_processPID != -1; });

    const auto onPidOfSetup = [this](Process &process) {
        process.setCommand(adbCommand({"shell", "pidof", m_packageName}));
    };
    const auto onPidOfDone = [this](const Process &process) {
        const QString pid = process.cleanedStdOut().trimmed();
        return pid == QString::number(m_processPID);
    };
    const auto pidOfTask = ProcessTask(onPidOfSetup, onPidOfDone, CallDoneIf::Success);

    const auto onRunAsSetup = [this](Process &process) {
        process.setCommand(adbCommand({"shell", "run-as", m_packageName, "kill", "-9",
                                       QString::number(m_processPID)}));
    };
    const auto runAsTask = ProcessTask(onRunAsSetup);

    const auto onKillSetup = [this](Process &process) {
        process.setCommand(adbCommand({"shell", "kill", "-9", QString::number(m_processPID)}));
    };

    return Group {
        ProcessTask(onForceStopSetup) || successItem,
        If (pidCheckSync && pidOfTask && !runAsTask) >> Then {
            ProcessTask(onKillSetup) || successItem
        }
    };
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
        process.setCommand(adbCommand({"--remove", port}));
    };
    const auto onForwardRemoveDone = [this](const Process &process) {
        emit remoteErrorOutput(process.cleanedStdErr().trimmed());
        return true;
    };

    const auto onForwardPortSetup = [this, port, adbArg](Process &process) {
        process.setCommand(adbCommand({"forward", port, adbArg}));
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

// The startBarrier is passed when logcat process received "Sending WAIT chunk" message.
// The settledBarrier is passed when logcat process received "debugger has settled" message.
ExecutableItem AndroidRunnerWorker::jdbRecipe(const SingleBarrier &startBarrier,
                                              const SingleBarrier &settledBarrier)
{
    const auto onSetup = [this] {
        return m_useCppDebugger ? SetupResult::Continue : SetupResult::StopWithSuccess;
    };

    const auto onTaskTreeSetup = [this](TaskTree &taskTree) {
        taskTree.setRecipe({
            removeForwardPortRecipe("tcp:" + s_localJdbServerPort.toString(),
                                    "jdwp:" + QString::number(m_processPID), "JDB")
        });
    };

    const auto onJdbSetup = [this, settledBarrier](Process &process) {
        const FilePath jdbPath = AndroidConfig::openJDKLocation().pathAppended("bin/jdb")
                                     .withExecutableSuffix();
        const QString portArg = QString("com.sun.jdi.SocketAttach:hostname=localhost,port=%1")
                                    .arg(s_localJdbServerPort.toString());
        process.setCommand({jdbPath, {"-connect", portArg}});
        process.setProcessMode(ProcessMode::Writer);
        process.setProcessChannelMode(QProcess::MergedChannels);
        process.setReaperTimeout(s_jdbTimeout);
        connect(settledBarrier->barrier(), &Barrier::done, &process, [processPtr = &process] {
            processPtr->write("ignore uncaught java.lang.Throwable\n"
                              "threads\n"
                              "cont\n"
                              "exit\n");
        });
    };
    const auto onJdbDone = [](const Process &process, DoneWith result) {
        qCDebug(androidRunWorkerLog) << qPrintable(process.allOutput());
        if (result == DoneWith::Cancel)
            qCCritical(androidRunWorkerLog) << "Terminating JDB due to timeout";
    };

    return Group {
        onGroupSetup(onSetup),
        waitForBarrierTask(startBarrier),
        TaskTreeTask(onTaskTreeSetup),
        ProcessTask(onJdbSetup, onJdbDone).withTimeout(60s)
    };
}

ExecutableItem AndroidRunnerWorker::logcatRecipe()
{
    struct Buffer {
        QStringList timeArgs;
        QByteArray stdOutBuffer;
        QByteArray stdErrBuffer;
    };

    const Storage<Buffer> storage;
    const SingleBarrier startJdbBarrier;   // When logcat received "Sending WAIT chunk".
    const SingleBarrier settledJdbBarrier; // When logcat received "debugger has settled".

    const auto onTimeSetup = [this](Process &process) {
        process.setCommand(adbCommand({"shell", "date", "+%s"}));
    };
    const auto onTimeDone = [storage](const Process &process) {
        storage->timeArgs = {"-T", QDateTime::fromSecsSinceEpoch(
            process.cleanedStdOut().trimmed().toInt()).toString("MM-dd hh:mm:ss.mmm")};
    };

    const auto onLogcatSetup = [this, storage, startJdbBarrier, settledJdbBarrier](Process &process) {
        Buffer *bufferPtr = storage.activeStorage();
        const auto parseLogcat = [this, bufferPtr, start = startJdbBarrier->barrier(),
                                  settled = settledJdbBarrier->barrier(), processPtr = &process](
                                     QProcess::ProcessChannel channel) {
            if (m_processPID == -1)
                return;

            QByteArray &buffer = channel == QProcess::StandardOutput ? bufferPtr->stdOutBuffer
                                                                     : bufferPtr->stdErrBuffer;
            const QByteArray &text = channel == QProcess::StandardOutput
                                         ? processPtr->readAllRawStandardOutput()
                                         : processPtr->readAllRawStandardError();
            QList<QByteArray> lines = text.split('\n');
            // lines always contains at least one item
            lines[0].prepend(buffer);
            if (lines.last().endsWith('\n'))
                buffer.clear();
            else
                buffer = lines.takeLast(); // incomplete line

            const QString pidString = QString::number(m_processPID);
            for (const QByteArray &msg : std::as_const(lines)) {
                const QString line = QString::fromUtf8(msg).trimmed() + QLatin1Char('\n');
                if (!line.contains(pidString))
                    continue;

                if (m_useCppDebugger) {
                    if (start->current() == 0 && msg.trimmed().endsWith("Sending WAIT chunk"))
                        start->advance();
                    else if (settled->current() == 0 && msg.indexOf("debugger has settled") > 0)
                        settled->advance();
                }

                static const QRegularExpression regExpLogcat{
                    "^[0-9\\-]*" // date
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
                    "[\\n\\r]*$"
                };

                const bool onlyError = channel == QProcess::StandardError;
                const QRegularExpressionMatch match = regExpLogcat.match(line);
                if (match.hasMatch()) {
                    // Android M
                    if (match.captured(1) == pidString) {
                        const QString msgType = match.captured(2);
                        const QString output = line.mid(match.capturedStart(2));
                        if (onlyError || msgType == "F" || msgType == "E" || msgType == "W")
                            emit remoteErrorOutput(output);
                        else
                            emit remoteOutput(output);
                    }
                } else {
                    if (onlyError || line.startsWith("F/") || line.startsWith("E/")
                        || line.startsWith("W/")) {
                        emit remoteErrorOutput(line);
                    } else {
                        emit remoteOutput(line);
                    }
                }
            }
        };
        connect(&process, &Process::readyReadStandardOutput, this, [parseLogcat] {
            parseLogcat(QProcess::StandardOutput);
        });
        connect(&process, &Process::readyReadStandardError, this, [parseLogcat] {
            parseLogcat(QProcess::StandardError);
        });
        process.setCommand(adbCommand({"logcat", storage->timeArgs}));
    };

    return Group {
        parallel,
        startJdbBarrier,
        settledJdbBarrier,
        Group {
            storage,
            ProcessTask(onTimeSetup, onTimeDone, CallDoneIf::Success) || successItem,
            ProcessTask(onLogcatSetup)
        },
        jdbRecipe(startJdbBarrier, settledJdbBarrier)
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
        process.setCommand(adbCommand({iterator->split(' ', Qt::SkipEmptyParts)}));
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
        process.setCommand(adbCommand({args}));
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

ExecutableItem AndroidRunnerWorker::postDoneRecipe()
{
    const LoopUntil iterator([this](int iteration) {
        return iteration < m_afterFinishAdbCommands.size();
    });

    const auto onProcessSetup = [this, iterator](Process &process) {
        process.setCommand(adbCommand(
            {m_afterFinishAdbCommands.at(iterator.iteration()).split(' ', Qt::SkipEmptyParts)}));
    };

    const auto onDone = [this] {
        m_processPID = -1;
        m_processUser = -1;
        emit remoteProcessFinished("\n\n" + Tr::tr("\"%1\" died.").arg(m_packageName));
        m_debugServerProcess.reset();
    };

    return Group {
        finishAllAndSuccess,
        For {
            iterator,
            ProcessTask(onProcessSetup)
        },
        onGroupDone(onDone)
    };
}

ExecutableItem AndroidRunnerWorker::pidRecipe()
{
    using PidUserPair = std::pair<qint64, qint64>;
    const Storage<PidUserPair> pidStorage;

    const QString pidScript = isPreNougat()
        ? QString("for p in /proc/[0-9]*; do cat <$p/cmdline && echo :${p##*/}; done")
        : QString("pidof -s '%1'").arg(m_packageName);

    const auto onPidSetup = [this, pidScript](Process &process) {
        process.setCommand(adbCommand({"shell", pidScript}));
    };
    const auto onPidDone = [pidStorage, packageName = m_packageName,
                            isPreNougat = isPreNougat()](const Process &process) {
        const QString out = process.allOutput();
        if (isPreNougat)
            pidStorage->first = extractPID(out, packageName);
        else if (!out.isEmpty())
            pidStorage->first = out.trimmed().toLongLong();
    };

    const auto onUserSetup = [this, pidStorage](Process &process) {
        process.setCommand(
            adbCommand({"shell", "ps", "-o", "user", "-p", QString::number(pidStorage->first)}));
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

    const auto onPidSync = [this, pidStorage] {
        qCDebug(androidRunWorkerLog) << "Process ID changed from:" << m_processPID
                                     << "to:" << pidStorage->first;
        m_processPID = pidStorage->first;
        m_processUser = pidStorage->second;
        emit remoteProcessStarted(s_localDebugServerPort, m_qmlServer, m_processPID);
    };

    const auto onIsAliveSetup = [this, pidStorage](Process &process) {
        process.setProcessChannelMode(QProcess::MergedChannels);
        process.setCommand(adbCommand({"shell", pidPollingScript.arg(pidStorage->first)}));
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
        Sync(onPidSync),
        Group {
            parallel,
            startNativeDebuggingRecipe(),
            ProcessTask(onIsAliveSetup)
        },
        postDoneRecipe()
    };
}

static QString tempDebugServerPath(int count)
{
    static const QString tempDebugServerPathTemplate = "/data/local/tmp/%1";
    return tempDebugServerPathTemplate.arg(count);
}

ExecutableItem AndroidRunnerWorker::uploadDebugServerRecipe(const QString &debugServerFileName)
{
    const Storage<QString> tempDebugServerPathStorage;
    const LoopUntil iterator([tempDebugServerPathStorage](int iteration) {
        return tempDebugServerPathStorage->isEmpty() && iteration <= GdbTempFileMaxCounter;
    });
    const auto onDeviceFileExistsSetup = [this, iterator](Process &process) {
        process.setCommand(
            adbCommand({"shell", "ls", tempDebugServerPath(iterator.iteration()), "2>/dev/null"}));
    };
    const auto onDeviceFileExistsDone = [iterator, tempDebugServerPathStorage](
                                            const Process &process, DoneWith result) {
        if (result == DoneWith::Error || process.stdOut().trimmed().isEmpty())
            *tempDebugServerPathStorage = tempDebugServerPath(iterator.iteration());
        return true;
    };
    const auto onTempDebugServerPath = [tempDebugServerPathStorage] {
        const bool tempDirOK = !tempDebugServerPathStorage->isEmpty();
        if (!tempDirOK)
            qCDebug(androidRunWorkerLog) << "Can not get temporary file name";
        return tempDirOK;
    };

    const auto onCleanupSetup = [this, tempDebugServerPathStorage](Process &process) {
        process.setCommand(adbCommand({"shell", "rm", "-f", *tempDebugServerPathStorage}));
    };
    const auto onCleanupDone = [] {
        qCDebug(androidRunWorkerLog) << "Debug server cleanup failed.";
    };

    const auto onServerUploadSetup = [this, tempDebugServerPathStorage](Process &process) {
        process.setCommand(
            adbCommand({"push", m_debugServerPath.toString(), *tempDebugServerPathStorage}));
    };

    const auto onServerCopySetup = [this, tempDebugServerPathStorage, debugServerFileName](Process &process) {
        process.setCommand(adbCommand({packageArgs(), "cp", *tempDebugServerPathStorage,
                                       debugServerFileName}));
    };

    const auto onServerChmodSetup = [this, debugServerFileName](Process &process) {
        process.setCommand(adbCommand({packageArgs(), "chmod", "777", debugServerFileName}));
    };

    return Group {
        tempDebugServerPathStorage,
        For {
            iterator,
            ProcessTask(onDeviceFileExistsSetup, onDeviceFileExistsDone)
        },
        Sync(onTempDebugServerPath),
        If (!ProcessTask(onServerUploadSetup)) >> Then {
            Sync([] { qCDebug(androidRunWorkerLog) << "Debug server upload to temp directory failed"; }),
            ProcessTask(onCleanupSetup, onCleanupDone, CallDoneIf::Error) && errorItem
        },
        If (!ProcessTask(onServerCopySetup)) >> Then {
            Sync([] { qCDebug(androidRunWorkerLog) << "Debug server copy from temp directory failed"; }),
            ProcessTask(onCleanupSetup, onCleanupDone, CallDoneIf::Error) && errorItem
        },
        ProcessTask(onServerChmodSetup),
        ProcessTask(onCleanupSetup, onCleanupDone, CallDoneIf::Error) || successItem
    };
}

ExecutableItem AndroidRunnerWorker::startNativeDebuggingRecipe()
{
    const auto onSetup = [this] {
        return m_useCppDebugger ? SetupResult::Continue : SetupResult::StopWithSuccess;
    };

    const Storage<QString> packageDirStorage;
    const Storage<QString> debugServerFileStorage;

    const auto onAppDirSetup = [this](Process &process) {
        process.setCommand(adbCommand({packageArgs(), "/system/bin/sh", "-c", "pwd"}));
    };
    const auto onAppDirDone = [this, packageDirStorage](const Process &process, DoneWith result) {
        if (result == DoneWith::Success)
            *packageDirStorage = process.stdOut();
        else
            emit remoteProcessFinished(Tr::tr("Failed to find application directory."));
    };

    // Add executable flag to package dir. Gdb can't connect to running server on device on
    // e.g. on Android 8 with NDK 10e
    const auto onChmodSetup = [this, packageDirStorage](Process &process) {
        process.setCommand(adbCommand({packageArgs(), "chmod", "a+x", packageDirStorage->trimmed()}));
    };
    const auto onServerPathCheck = [this] {
        if (m_debugServerPath.exists())
            return true;
        QString msg = Tr::tr("Cannot find C++ debug server in NDK installation.");
        if (m_useLldb)
            msg += "\n" + Tr::tr("The lldb-server binary has not been found.");
        emit remoteProcessFinished(msg);
        return false;
    };

    const auto useLldb = Sync([this] { return m_useLldb; });
    const auto killAll = [this](const QString &name) {
        return ProcessTask([this, name](Process &process) {
            process.setCommand(adbCommand({packageArgs(), "killall", name}));
        }) || successItem;
    };
    const auto setDebugServer = [debugServerFileStorage](const QString &fileName) {
        return Sync([debugServerFileStorage, fileName] { *debugServerFileStorage = fileName; });
    };

    const auto uploadDebugServer = [this, setDebugServer](const QString &debugServerFileName) {
        return If (uploadDebugServerRecipe(debugServerFileName)) >> Then {
            setDebugServer(debugServerFileName)
        } >> Else {
            Sync([this] {
                emit remoteProcessFinished(Tr::tr("Cannot copy C++ debug server."));
                return false;
            })
        };
    };

    const auto packageFileExists = [this](const QString &filePath) {
        const auto onProcessSetup = [this, filePath](Process &process) {
            process.setCommand(adbCommand({packageArgs(), "ls", filePath, "2>/dev/null"}));
        };
        const auto onProcessDone = [this](const Process &process) {
            return !process.stdOut().trimmed().isEmpty();
        };
        return ProcessTask(onProcessSetup, onProcessDone, CallDoneIf::Success);
    };

    const auto onRemoveGdbServerSetup = [this, packageDirStorage](Process &process) {
        const QString gdbServerSocket = *packageDirStorage + "/debug-socket";
        process.setCommand(adbCommand({packageArgs(), "rm", gdbServerSocket}));
    };

    const auto onDebugServerSetup = [this, packageDirStorage, debugServerFileStorage](Process &process) {
        if (m_useLldb) {
            process.setCommand(adbCommand({packageArgs(), *debugServerFileStorage, "platform",
                               "--listen", QString("*:%1").arg(s_localDebugServerPort.toString())}));
        } else {
            const QString gdbServerSocket = *packageDirStorage + "/debug-socket";
            process.setCommand(adbCommand({packageArgs(), *debugServerFileStorage, "--multi",
                                           QString("+%1").arg(gdbServerSocket)}));
        }
    };

    const auto onTaskTreeSetup = [this, packageDirStorage](TaskTree &taskTree) {
        const QString gdbServerSocket = *packageDirStorage + "/debug-socket";
        taskTree.setRecipe({removeForwardPortRecipe("tcp:" + s_localDebugServerPort.toString(),
                                                    "localfilesystem:" + gdbServerSocket, "C++")});
    };

    return Group {
        packageDirStorage,
        debugServerFileStorage,
        onGroupSetup(onSetup),
        ProcessTask(onAppDirSetup, onAppDirDone),
        ProcessTask(onChmodSetup) || successItem,
        Sync(onServerPathCheck),
        If (useLldb) >> Then {
            killAll("lldb-server"),
            uploadDebugServer("./lldb-server")
        } >> ElseIf (packageFileExists("./lib/gdbserver")) >> Then {
            killAll("gdbserver"),
            setDebugServer("./lib/gdbserver")
        } >> ElseIf (packageFileExists("./lib/libgdbserver.so")) >> Then {
            killAll("libgdbserver.so"),
            setDebugServer("./lib/libgdbserver.so")
        } >> Else {
            killAll("gdbserver"),
            uploadDebugServer("./gdbserver")
        },
        If (!useLldb) >> Then {
            ProcessTask(onRemoveGdbServerSetup) || successItem
        },
        Group {
            parallel,
            ProcessTask(onDebugServerSetup),
            If (!useLldb) >> Then {
                TaskTreeTask(onTaskTreeSetup)
            }
        }
    };
}

void AndroidRunnerWorker::asyncStart()
{
    const Group recipe {
        forceStopRecipe(),
        Group {
            parallel,
            logcatRecipe(),
            Group {
                preStartRecipe(),
                pidRecipe()
            }
        }
    };
    m_taskTreeRunner.start(recipe);
}

void AndroidRunnerWorker::asyncStop()
{
    if (m_processPID != -1)
        m_taskTreeRunner.start(Group { forceStopRecipe(), postDoneRecipe() });
    else
        m_taskTreeRunner.reset();
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

} // namespace Android::Internal
