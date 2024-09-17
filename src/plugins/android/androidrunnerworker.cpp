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
#include <projectexplorer/runcontrol.h>
#include <projectexplorer/target.h>

#include <qmldebug/qmldebugcommandlinearguments.h>

#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitaspect.h>

#include <solutions/tasking/barrier.h>
#include <solutions/tasking/conditional.h>

#include <utils/hostosinfo.h>
#include <utils/port.h>
#include <utils/qtcprocess.h>
#include <utils/url.h>

#include <QDateTime>
#include <QLoggingCategory>
#include <QRegularExpression>
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

class RunnerStorage
{
public:
    bool isPreNougat() const { return m_glue->apiLevel() > 0 && m_glue->apiLevel() <= 23; }
    Utils::CommandLine adbCommand(std::initializer_list<Utils::CommandLine::ArgRef> args) const
    {
        CommandLine cmd{AndroidConfig::adbToolPath(), args};
        cmd.prependArgs(AndroidDeviceInfo::adbSelector(m_glue->deviceSerialNumber()));
        return cmd;
    }
    QStringList userArgs() const
    {
        return m_processUser > 0 ? QStringList{"--user", QString::number(m_processUser)} : QStringList{};
    }
    QStringList packageArgs() const
    {
        // run-as <package-name> pwd fails on API 22 so route the pwd through shell.
        return QStringList{"shell", "run-as", m_packageName} + userArgs();
    }

    RunnerInterface *m_glue = nullptr;

    QString m_packageName;
    QString m_intentName;
    QStringList m_beforeStartAdbCommands;
    QStringList m_afterFinishAdbCommands;
    QStringList m_amStartExtraArgs;
    qint64 m_processPID = -1;
    qint64 m_processUser = -1;
    bool m_useCppDebugger = false;
    bool m_useLldb = false;
    QmlDebug::QmlDebugServicesPreset m_qmlDebugServices;
    QUrl m_qmlServer;
    QString m_extraAppParams;
    Utils::Environment m_extraEnvVars;
    Utils::FilePath m_debugServerPath; // On build device, typically as part of ndk
    bool m_useAppParamsForQmlDebugger = false;
};

static void setupStorage(RunnerStorage *storage, RunnerInterface *glue)
{
    storage->m_glue = glue;
    storage->m_useLldb = Debugger::DebuggerKitAspect::engineType(glue->runControl()->kit())
                         == Debugger::LldbEngineType;
    auto aspect = glue->runControl()->aspectData<Debugger::DebuggerRunConfigurationAspect>();
    const Id runMode = glue->runControl()->runMode();
    const bool debuggingMode = runMode == ProjectExplorer::Constants::DEBUG_RUN_MODE;
    storage->m_useCppDebugger = debuggingMode && aspect->useCppDebugger;
    if (debuggingMode && aspect->useQmlDebugger)
        storage->m_qmlDebugServices = QmlDebug::QmlDebuggerServices;
    else if (runMode == ProjectExplorer::Constants::QML_PROFILER_RUN_MODE)
        storage->m_qmlDebugServices = QmlDebug::QmlProfilerServices;
    else if (runMode == ProjectExplorer::Constants::QML_PREVIEW_RUN_MODE)
        storage->m_qmlDebugServices = QmlDebug::QmlPreviewServices;
    else
        storage->m_qmlDebugServices = QmlDebug::NoQmlDebugServices;

    if (storage->m_qmlDebugServices != QmlDebug::NoQmlDebugServices) {
        qCDebug(androidRunWorkerLog) << "QML debugging enabled";
        QTcpServer server;
        const bool isListening = server.listen(QHostAddress::LocalHost);
        QTC_ASSERT(isListening,
                   qDebug() << Tr::tr("No free ports available on host for QML debugging."));
        storage->m_qmlServer.setScheme(Utils::urlTcpScheme());
        storage->m_qmlServer.setHost(server.serverAddress().toString());
        storage->m_qmlServer.setPort(server.serverPort());
        qCDebug(androidRunWorkerLog) << "QML server:" << storage->m_qmlServer.toDisplayString();
    }

    auto target = glue->runControl()->target();
    storage->m_packageName = AndroidManager::packageName(target);
    storage->m_intentName = storage->m_packageName + '/' + AndroidManager::activityName(target);
    qCDebug(androidRunWorkerLog) << "Intent name:" << storage->m_intentName
                                 << "Package name:" << storage->m_packageName;
    qCDebug(androidRunWorkerLog) << "Device API:" << glue->apiLevel();

    storage->m_extraEnvVars = glue->runControl()->aspectData<EnvironmentAspect>()->environment;
    qCDebug(androidRunWorkerLog).noquote() << "Environment variables for the app"
                                           << storage->m_extraEnvVars.toStringList();

    if (target->buildConfigurations().first()->buildType() != BuildConfiguration::BuildType::Release)
        storage->m_extraAppParams = glue->runControl()->commandLine().arguments();

    if (const Store sd = glue->runControl()->settingsData(Constants::ANDROID_AM_START_ARGS);
        !sd.isEmpty()) {
        QTC_CHECK(sd.first().typeId() == QMetaType::QString);
        const QString startArgs = sd.first().toString();
        storage->m_amStartExtraArgs = ProcessArgs::splitArgs(startArgs, OsTypeOtherUnix);
    }

    if (const Store sd = glue->runControl()->settingsData(Constants::ANDROID_PRESTARTSHELLCMDLIST);
        !sd.isEmpty()) {
        const QVariant &first = sd.first();
        QTC_CHECK(first.typeId() == QMetaType::QStringList);
        const QStringList commands = first.toStringList();
        for (const QString &shellCmd : commands)
            storage->m_beforeStartAdbCommands.append(QString("shell %1").arg(shellCmd));
    }

    if (const Store sd = glue->runControl()->settingsData(Constants::ANDROID_POSTFINISHSHELLCMDLIST);
        !sd.isEmpty()) {
        const QVariant &first = sd.first();
        QTC_CHECK(first.typeId() == QMetaType::QStringList);
        const QStringList commands = first.toStringList();
        for (const QString &shellCmd : commands)
            storage->m_afterFinishAdbCommands.append(QString("shell %1").arg(shellCmd));
    }

    storage->m_debugServerPath = debugServer(storage->m_useLldb, target);
    qCDebug(androidRunWorkerLog).noquote() << "Device Serial:" << glue->deviceSerialNumber()
                                           << ", API level:" << glue->apiLevel()
                                           << ", Extra Start Args:" << storage->m_amStartExtraArgs
                                           << ", Before Start ADB cmds:" << storage->m_beforeStartAdbCommands
                                           << ", After finish ADB cmds:" << storage->m_afterFinishAdbCommands
                                           << ", Debug server path:" << storage->m_debugServerPath;

    QtSupport::QtVersion *version = QtSupport::QtKitAspect::qtVersion(target->kit());
    storage->m_useAppParamsForQmlDebugger = version->qtVersion() >= QVersionNumber(5, 12);
}

static ExecutableItem forceStopRecipe(const Storage<RunnerStorage> &storage)
{
    const auto onForceStopSetup = [storage](Process &process) {
        process.setCommand(storage->adbCommand({"shell", "am", "force-stop", storage->m_packageName}));
    };

    const auto pidCheckSync = Sync([storage] { return storage->m_processPID != -1; });

    const auto onPidOfSetup = [storage](Process &process) {
        process.setCommand(storage->adbCommand({"shell", "pidof", storage->m_packageName}));
    };
    const auto onPidOfDone = [storage](const Process &process) {
        const QString pid = process.cleanedStdOut().trimmed();
        return pid == QString::number(storage->m_processPID);
    };
    const auto pidOfTask = ProcessTask(onPidOfSetup, onPidOfDone, CallDoneIf::Success);

    const auto onRunAsSetup = [storage](Process &process) {
        process.setCommand(storage->adbCommand({"shell", "run-as", storage->m_packageName, "kill", "-9",
                                       QString::number(storage->m_processPID)}));
    };
    const auto runAsTask = ProcessTask(onRunAsSetup);

    const auto onKillSetup = [storage](Process &process) {
        process.setCommand(storage->adbCommand({"shell", "kill", "-9",
                                                QString::number(storage->m_processPID)}));
    };

    return Group {
        ProcessTask(onForceStopSetup) || successItem,
        If (pidCheckSync && pidOfTask && !runAsTask) >> Then {
            ProcessTask(onKillSetup) || successItem
        }
    };
}

static ExecutableItem removeForwardPortRecipe(RunnerStorage *storage, const QString &port,
                                              const QString &adbArg, const QString &portType)
{
    const auto onForwardListSetup = [](Process &process) {
        process.setCommand({AndroidConfig::adbToolPath(), {"forward", "--list"}});
    };
    const auto onForwardListDone = [port](const Process &process) {
        return process.cleanedStdOut().trimmed().contains(port);
    };

    const auto onForwardRemoveSetup = [storage, port](Process &process) {
        process.setCommand(storage->adbCommand({"forward", "--remove", port}));
    };
    const auto onForwardRemoveDone = [storage](const Process &process) {
        storage->m_glue->addStdErr(process.cleanedStdErr().trimmed());
        return true;
    };

    const auto onForwardPortSetup = [storage, port, adbArg](Process &process) {
        process.setCommand(storage->adbCommand({"forward", port, adbArg}));
    };
    const auto onForwardPortDone = [storage, port, portType](DoneWith result) {
        if (result == DoneWith::Success)
            storage->m_afterFinishAdbCommands.push_back("forward --remove " + port);
        else
            storage->m_glue->setFinished(Tr::tr("Failed to forward %1 debugging ports.").arg(portType));
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
static ExecutableItem jdbRecipe(const Storage<RunnerStorage> &storage,
                                const SingleBarrier &startBarrier,
                                const SingleBarrier &settledBarrier)
{
    const auto onSetup = [storage] {
        return storage->m_useCppDebugger ? SetupResult::Continue : SetupResult::StopWithSuccess;
    };

    const auto onTaskTreeSetup = [storage](TaskTree &taskTree) {
        taskTree.setRecipe({removeForwardPortRecipe(storage.activeStorage(),
                            "tcp:" + s_localJdbServerPort.toString(),
                            "jdwp:" + QString::number(storage->m_processPID), "JDB")
        });
    };

    const auto onJdbSetup = [settledBarrier](Process &process) {
        const FilePath jdbPath = AndroidConfig::openJDKLocation().pathAppended("bin/jdb")
                                     .withExecutableSuffix();
        const QString portArg = QString("com.sun.jdi.SocketAttach:hostname=localhost,port=%1")
                                    .arg(s_localJdbServerPort.toString());
        process.setCommand({jdbPath, {"-connect", portArg}});
        process.setProcessMode(ProcessMode::Writer);
        process.setProcessChannelMode(QProcess::MergedChannels);
        process.setReaperTimeout(s_jdbTimeout);
        QObject::connect(settledBarrier->barrier(), &Barrier::done, &process, [processPtr = &process] {
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

static ExecutableItem logcatRecipe(const Storage<RunnerStorage> &storage)
{
    struct Buffer {
        QStringList timeArgs;
        QByteArray stdOutBuffer;
        QByteArray stdErrBuffer;
    };

    const Storage<Buffer> bufferStorage;
    const SingleBarrier startJdbBarrier;   // When logcat received "Sending WAIT chunk".
    const SingleBarrier settledJdbBarrier; // When logcat received "debugger has settled".

    const auto onTimeSetup = [storage](Process &process) {
        process.setCommand(storage->adbCommand({"shell", "date", "+%s"}));
    };
    const auto onTimeDone = [bufferStorage](const Process &process) {
        bufferStorage->timeArgs = {"-T", QDateTime::fromSecsSinceEpoch(
            process.cleanedStdOut().trimmed().toInt()).toString("MM-dd hh:mm:ss.mmm")};
    };

    const auto onLogcatSetup = [storage, bufferStorage, startJdbBarrier, settledJdbBarrier](Process &process) {
        RunnerStorage *storagePtr = storage.activeStorage();
        Buffer *bufferPtr = bufferStorage.activeStorage();
        const auto parseLogcat = [storagePtr, bufferPtr, start = startJdbBarrier->barrier(),
                                  settled = settledJdbBarrier->barrier(), processPtr = &process](
                                     QProcess::ProcessChannel channel) {
            if (storagePtr->m_processPID == -1)
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

            const QString pidString = QString::number(storagePtr->m_processPID);
            for (const QByteArray &msg : std::as_const(lines)) {
                const QString line = QString::fromUtf8(msg).trimmed() + QLatin1Char('\n');
                if (!line.contains(pidString))
                    continue;

                if (storagePtr->m_useCppDebugger) {
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
                            storagePtr->m_glue->addStdErr(output);
                        else
                            storagePtr->m_glue->addStdOut(output);
                    }
                } else {
                    if (onlyError || line.startsWith("F/") || line.startsWith("E/")
                        || line.startsWith("W/")) {
                        storagePtr->m_glue->addStdErr(line);
                    } else {
                        storagePtr->m_glue->addStdOut(line);
                    }
                }
            }
        };
        QObject::connect(&process, &Process::readyReadStandardOutput, &process, [parseLogcat] {
            parseLogcat(QProcess::StandardOutput);
        });
        QObject::connect(&process, &Process::readyReadStandardError, &process, [parseLogcat] {
            parseLogcat(QProcess::StandardError);
        });
        process.setCommand(storage->adbCommand({"logcat", bufferStorage->timeArgs}));
    };

    return Group {
        parallel,
        startJdbBarrier,
        settledJdbBarrier,
        Group {
            bufferStorage,
            ProcessTask(onTimeSetup, onTimeDone, CallDoneIf::Success) || successItem,
            ProcessTask(onLogcatSetup)
        },
        jdbRecipe(storage, startJdbBarrier, settledJdbBarrier)
    };
}

static ExecutableItem preStartRecipe(const Storage<RunnerStorage> &storage)
{
    const Storage<QStringList> argsStorage;
    const LoopUntil iterator([storage](int iteration) {
        return iteration < storage->m_beforeStartAdbCommands.size();
    });

    const auto onArgsSetup = [storage, argsStorage] {
        *argsStorage = {"shell", "am", "start", "-n", storage->m_intentName};
        if (storage->m_useCppDebugger)
            *argsStorage << "-D";
    };

    const auto onPreCommandSetup = [storage, iterator](Process &process) {
        process.setCommand(storage->adbCommand(
            {storage->m_beforeStartAdbCommands.at(iterator.iteration()).split(' ', Qt::SkipEmptyParts)}));
    };
    const auto onPreCommandDone = [storage](const Process &process) {
        storage->m_glue->addStdErr(process.cleanedStdErr().trimmed());
    };

    const auto onQmlDebugSetup = [storage] {
        return storage->m_qmlDebugServices == QmlDebug::NoQmlDebugServices
                                            ? SetupResult::StopWithSuccess : SetupResult::Continue;
    };
    const auto onTaskTreeSetup = [storage](TaskTree &taskTree) {
        const QString port = "tcp:" + QString::number(storage->m_qmlServer.port());
        taskTree.setRecipe({removeForwardPortRecipe(storage.activeStorage(), port, port, "QML")});
    };
    const auto onQmlDebugDone = [storage, argsStorage] {
        const QString qmljsdebugger = QString("port:%1,block,services:%2")
            .arg(storage->m_qmlServer.port()).arg(QmlDebug::qmlDebugServices(storage->m_qmlDebugServices));

        if (storage->m_useAppParamsForQmlDebugger) {
            if (!storage->m_extraAppParams.isEmpty())
                storage->m_extraAppParams.prepend(' ');
            storage->m_extraAppParams.prepend("-qmljsdebugger=" + qmljsdebugger);
        } else {
            *argsStorage << "-e" << "qml_debug" << "true"
                         << "-e" << "qmljsdebugger" << qmljsdebugger;
        }
    };

    const auto onActivitySetup = [storage, argsStorage](Process &process) {
        QStringList args = *argsStorage;
        args << storage->m_amStartExtraArgs;

        if (!storage->m_extraAppParams.isEmpty()) {
            const QStringList appArgs =
                ProcessArgs::splitArgs(storage->m_extraAppParams, Utils::OsType::OsTypeLinux);
            qCDebug(androidRunWorkerLog).noquote() << "Using application arguments: " << appArgs;
            args << "-e" << "extraappparams"
                 << QString::fromLatin1(appArgs.join(' ').toUtf8().toBase64());
        }

        if (storage->m_extraEnvVars.hasChanges()) {
            args << "-e" << "extraenvvars"
                 << QString::fromLatin1(storage->m_extraEnvVars.toStringList().join('\t')
                                            .toUtf8().toBase64());
        }
        process.setCommand(storage->adbCommand({args}));
    };
    const auto onActivityDone = [storage](const Process &process) {
        storage->m_glue->setFinished(
            Tr::tr("Activity Manager error: %1").arg(process.cleanedStdErr().trimmed()));
    };

    return Group {
        argsStorage,
        onGroupSetup(onArgsSetup),
        For (iterator) >> Do {
            ProcessTask(onPreCommandSetup, onPreCommandDone, CallDoneIf::Error)
        },
        Group {
            onGroupSetup(onQmlDebugSetup),
            TaskTreeTask(onTaskTreeSetup),
            onGroupDone(onQmlDebugDone, CallDoneIf::Success)
        },
        ProcessTask(onActivitySetup, onActivityDone, CallDoneIf::Error)
    };
}

static ExecutableItem postDoneRecipe(const Storage<RunnerStorage> &storage)
{
    const LoopUntil iterator([storage](int iteration) {
        return iteration < storage->m_afterFinishAdbCommands.size();
    });

    const auto onProcessSetup = [storage, iterator](Process &process) {
        process.setCommand(storage->adbCommand(
            {storage->m_afterFinishAdbCommands.at(iterator.iteration()).split(' ', Qt::SkipEmptyParts)}));
    };

    const auto onDone = [storage] {
        storage->m_processPID = -1;
        storage->m_processUser = -1;
        storage->m_glue->setFinished("\n\n" + Tr::tr("\"%1\" died.").arg(storage->m_packageName));
    };

    return Group {
        finishAllAndSuccess,
        For (iterator) >> Do {
            ProcessTask(onProcessSetup)
        },
        onGroupDone(onDone)
    };
}

static QString tempDebugServerPath(int count)
{
    static const QString tempDebugServerPathTemplate = "/data/local/tmp/%1";
    return tempDebugServerPathTemplate.arg(count);
}

static ExecutableItem uploadDebugServerRecipe(const Storage<RunnerStorage> &storage,
                                              const QString &debugServerFileName)
{
    const Storage<QString> tempDebugServerPathStorage;
    const LoopUntil iterator([tempDebugServerPathStorage](int iteration) {
        return tempDebugServerPathStorage->isEmpty() && iteration <= GdbTempFileMaxCounter;
    });
    const auto onDeviceFileExistsSetup = [storage, iterator](Process &process) {
        process.setCommand(
            storage->adbCommand({"shell", "ls", tempDebugServerPath(iterator.iteration()), "2>/dev/null"}));
    };
    const auto onDeviceFileExistsDone = [iterator, tempDebugServerPathStorage](
                                            const Process &process, DoneWith result) {
        if (result == DoneWith::Error || process.stdOut().trimmed().isEmpty())
            *tempDebugServerPathStorage = tempDebugServerPath(iterator.iteration());
        return true;
    };
    const auto onTempDebugServerPath = [storage, tempDebugServerPathStorage] {
        const bool tempDirOK = !tempDebugServerPathStorage->isEmpty();
        if (tempDirOK) {
            storage->m_glue->setStarted(s_localDebugServerPort, storage->m_qmlServer,
                                        storage->m_processPID);
        } else {
            qCDebug(androidRunWorkerLog) << "Can not get temporary file name";
        }
        return tempDirOK;
    };

    const auto onCleanupSetup = [storage, tempDebugServerPathStorage](Process &process) {
        process.setCommand(storage->adbCommand({"shell", "rm", "-f", *tempDebugServerPathStorage}));
    };
    const auto onCleanupDone = [] {
        qCDebug(androidRunWorkerLog) << "Debug server cleanup failed.";
    };

    const auto onServerUploadSetup = [storage, tempDebugServerPathStorage](Process &process) {
        process.setCommand(storage->adbCommand(
            {"push", storage->m_debugServerPath.path(), *tempDebugServerPathStorage}));
    };

    const auto onServerCopySetup = [storage, tempDebugServerPathStorage, debugServerFileName](Process &process) {
        process.setCommand(storage->adbCommand({storage->packageArgs(), "cp",
                                                *tempDebugServerPathStorage, debugServerFileName}));
    };

    const auto onServerChmodSetup = [storage, debugServerFileName](Process &process) {
        process.setCommand(storage->adbCommand({storage->packageArgs(), "chmod", "777", debugServerFileName}));
    };

    return Group {
        tempDebugServerPathStorage,
        For (iterator) >> Do {
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

static ExecutableItem startNativeDebuggingRecipe(const Storage<RunnerStorage> &storage)
{
    const auto onSetup = [storage] {
        return storage->m_useCppDebugger ? SetupResult::Continue : SetupResult::StopWithSuccess;
    };

    const Storage<QString> packageDirStorage;
    const Storage<QString> debugServerFileStorage;

    const auto onAppDirSetup = [storage](Process &process) {
        process.setCommand(storage->adbCommand({storage->packageArgs(), "/system/bin/sh", "-c", "pwd"}));
    };
    const auto onAppDirDone = [storage, packageDirStorage](const Process &process, DoneWith result) {
        if (result == DoneWith::Success)
            *packageDirStorage = process.stdOut();
        else
            storage->m_glue->setFinished(Tr::tr("Failed to find application directory."));
    };

    // Add executable flag to package dir. Gdb can't connect to running server on device on
    // e.g. on Android 8 with NDK 10e
    const auto onChmodSetup = [storage, packageDirStorage](Process &process) {
        process.setCommand(storage->adbCommand({storage->packageArgs(), "chmod", "a+x", packageDirStorage->trimmed()}));
    };
    const auto onServerPathCheck = [storage] {
        if (storage->m_debugServerPath.exists())
            return true;
        QString msg = Tr::tr("Cannot find C++ debug server in NDK installation.");
        if (storage->m_useLldb)
            msg += "\n" + Tr::tr("The lldb-server binary has not been found.");
        storage->m_glue->setFinished(msg);
        return false;
    };

    const auto useLldb = Sync([storage] { return storage->m_useLldb; });
    const auto killAll = [storage](const QString &name) {
        return ProcessTask([storage, name](Process &process) {
                   process.setCommand(storage->adbCommand({storage->packageArgs(), "killall", name}));
               }) || successItem;
    };
    const auto setDebugServer = [debugServerFileStorage](const QString &fileName) {
        return Sync([debugServerFileStorage, fileName] { *debugServerFileStorage = fileName; });
    };

    const auto uploadDebugServer = [storage, setDebugServer](const QString &debugServerFileName) {
        return If (uploadDebugServerRecipe(storage, debugServerFileName)) >> Then {
            setDebugServer(debugServerFileName)
        } >> Else {
            Sync([storage] {
                storage->m_glue->setFinished(Tr::tr("Cannot copy C++ debug server."));
                return false;
            })
        };
    };

    const auto packageFileExists = [storage](const QString &filePath) {
        const auto onProcessSetup = [storage, filePath](Process &process) {
            process.setCommand(storage->adbCommand({storage->packageArgs(), "ls", filePath, "2>/dev/null"}));
        };
        const auto onProcessDone = [](const Process &process) {
            return !process.stdOut().trimmed().isEmpty();
        };
        return ProcessTask(onProcessSetup, onProcessDone, CallDoneIf::Success);
    };

    const auto onRemoveGdbServerSetup = [storage, packageDirStorage](Process &process) {
        const QString gdbServerSocket = *packageDirStorage + "/debug-socket";
        process.setCommand(storage->adbCommand({storage->packageArgs(), "rm", gdbServerSocket}));
    };

    const auto onDebugServerSetup = [storage, packageDirStorage, debugServerFileStorage](Process &process) {
        if (storage->m_useLldb) {
            process.setCommand(storage->adbCommand({storage->packageArgs(), *debugServerFileStorage, "platform",
                                                    "--listen", QString("*:%1").arg(s_localDebugServerPort.toString())}));
        } else {
            const QString gdbServerSocket = *packageDirStorage + "/debug-socket";
            process.setCommand(storage->adbCommand({storage->packageArgs(), *debugServerFileStorage, "--multi",
                                                    QString("+%1").arg(gdbServerSocket)}));
        }
    };

    const auto onTaskTreeSetup = [storage, packageDirStorage](TaskTree &taskTree) {
        const QString gdbServerSocket = *packageDirStorage + "/debug-socket";
        taskTree.setRecipe({removeForwardPortRecipe(storage.activeStorage(),
                                                    "tcp:" + s_localDebugServerPort.toString(),
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

static ExecutableItem pidRecipe(const Storage<RunnerStorage> &storage)
{
    const auto onPidSetup = [storage](Process &process) {
        const QString pidScript = storage->isPreNougat()
            ? QString("for p in /proc/[0-9]*; do cat <$p/cmdline && echo :${p##*/}; done")
            : QString("pidof -s '%1'").arg(storage->m_packageName);
        process.setCommand(storage->adbCommand({"shell", pidScript}));
    };
    const auto onPidDone = [storage](const Process &process) {
        const QString out = process.allOutput();
        if (storage->isPreNougat())
            storage->m_processPID = extractPID(out, storage->m_packageName);
        else if (!out.isEmpty())
            storage->m_processPID = out.trimmed().toLongLong();
    };

    const auto onUserSetup = [storage](Process &process) {
        process.setCommand(storage->adbCommand(
            {"shell", "ps", "-o", "user", "-p", QString::number(storage->m_processPID)}));
    };
    const auto onUserDone = [storage](const Process &process) {
        const QString out = process.allOutput();
        if (out.isEmpty())
            return DoneResult::Error;

        QRegularExpressionMatch match;
        qsizetype matchPos = out.indexOf(userIdPattern, 0, &match);
        if (matchPos >= 0 && match.capturedLength(1) > 0) {
            bool ok = false;
            const qint64 processUser = match.captured(1).toInt(&ok);
            if (ok) {
                storage->m_processUser = processUser;
                qCDebug(androidRunWorkerLog) << "Process ID changed to:" << storage->m_processPID;
                if (!storage->m_useCppDebugger) {
                    storage->m_glue->setStarted(s_localDebugServerPort, storage->m_qmlServer,
                                                storage->m_processPID);
                }
                return DoneResult::Success;
            }
        }
        return DoneResult::Error;
    };

    const auto onArtSetup = [storage](Process &process) {
        process.setCommand(storage->adbCommand({"shell", "pm", "art", "clear-app-profiles",
                                                storage->m_packageName}));
    };

    const auto onCompileSetup = [storage](Process &process) {
        process.setCommand(storage->adbCommand({"shell", "pm", "compile", "-m", "verify", "-f",
                                                storage->m_packageName}));
    };

    const auto onIsAliveSetup = [storage](Process &process) {
        process.setProcessChannelMode(QProcess::MergedChannels);
        process.setCommand(storage->adbCommand({"shell", pidPollingScript.arg(storage->m_processPID)}));
    };

    return Group {
        Forever {
            stopOnSuccess,
            ProcessTask(onPidSetup, onPidDone, CallDoneIf::Success),
            TimeoutTask([](std::chrono::milliseconds &timeout) { timeout = 200ms; },
                        DoneResult::Error)
        }.withTimeout(45s),
        ProcessTask(onUserSetup, onUserDone, CallDoneIf::Success),
        ProcessTask(onArtSetup, DoneResult::Success),
        ProcessTask(onCompileSetup, DoneResult::Success),
        Group {
            parallel,
            startNativeDebuggingRecipe(storage),
            ProcessTask(onIsAliveSetup)
        }
    };
}

void RunnerInterface::setStarted(const Utils::Port &debugServerPort, const QUrl &qmlServer,
                                 qint64 pid)
{
    emit started(debugServerPort, qmlServer, pid);
}

ExecutableItem runnerRecipe(const Storage<RunnerInterface> &glueStorage)
{
    const Storage<RunnerStorage> storage;

    const auto onSetup = [glueStorage, storage] {
        setupStorage(storage.activeStorage(), glueStorage.activeStorage());
    };

    return Group {
        finishAllAndSuccess,
        storage,
        onGroupSetup(onSetup),
        Group {
            forceStopRecipe(storage),
            Group {
                parallel,
                stopOnSuccessOrError,
                logcatRecipe(storage),
                Group {
                    preStartRecipe(storage),
                    pidRecipe(storage)
                }
            }
        }.withCancel([glueStorage] {
            return std::make_pair(glueStorage.activeStorage(), &RunnerInterface::canceled);
        }),
        forceStopRecipe(storage),
        postDoneRecipe(storage)
    };
}

} // namespace Android::Internal
