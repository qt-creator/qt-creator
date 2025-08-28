// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "iosrunner.h"

#include "devicectlutils.h"
#include "iosconfigurations.h"
#include "iosconstants.h"
#include "iosdevice.h"
#include "iosrunconfiguration.h"
#include "iossimulator.h"
#include "iostoolhandler.h"
#include "iostr.h"

#include <debugger/debuggerconstants.h>
#include <debugger/debuggerkitaspect.h>
#include <debugger/debuggerruncontrol.h>

#include <projectexplorer/devicesupport/devicekitaspects.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/qmldebugcommandlinearguments.h>
#include <projectexplorer/runconfigurationaspects.h>
#include <projectexplorer/target.h>
#include <projectexplorer/taskhub.h>
#include <projectexplorer/toolchain.h>

#include <utils/fileutils.h>
#include <utils/qtcprocess.h>
#include <utils/stringutils.h>
#include <utils/temporaryfile.h>
#include <utils/url.h>
#include <utils/utilsicons.h>

#include <solutions/tasking/barrier.h>
#include <solutions/tasking/tasktree.h>

#include <QDateTime>
#include <QDir>
#include <QJsonArray>
#include <QMessageBox>
#include <QRegularExpression>
#include <QSettings>
#include <QTcpServer>
#include <QTime>
#include <QTimer>

#include <memory>

#include <fcntl.h>
#ifdef Q_OS_UNIX
#include <unistd.h>
#else
#include <io.h>
#endif

using namespace Debugger;
using namespace ProjectExplorer;
using namespace Utils;
using namespace Tasking;

namespace Ios::Internal {

char QML_DEBUGGER_WAITING[] = "QML Debugger: Waiting for connection on port ([0-9]+)...";

static QString identifierForRunControl(RunControl *runControl)
{
    const IosDeviceTypeAspect::Data *data = runControl->aspectData<IosDeviceTypeAspect>();
    return data ? data->deviceType.identifier : QString();
}

static void stopRunningRunControl(RunControl *runControl)
{
    static QMap<Id, QPointer<RunControl>> activeRunControls;

    // clean up deleted
    Utils::erase(activeRunControls, [](const QPointer<RunControl> &rc) { return !rc; });

    const Id devId = RunDeviceKitAspect::deviceId(runControl->kit());
    const QString identifier = identifierForRunControl(runControl);

    // The device can only run an application at a time, if an app is running stop it.
    if (QPointer<RunControl> activeRunControl = activeRunControls[devId]) {
        if (identifierForRunControl(activeRunControl) == identifier) {
            activeRunControl->initiateStop();
            activeRunControls.remove(devId);
        }
    }

    if (devId.isValid())
        activeRunControls[devId] = runControl;
}

static QString getBundleIdentifier(const FilePath &bundlePath)
{
    QSettings settings(bundlePath.pathAppended("Info.plist").toUrlishString(), QSettings::NativeFormat);
    return settings.value(QString::fromLatin1("CFBundleIdentifier")).toString();
}

struct AppInfo
{
    QUrl pathOnDevice;
    qint64 processIdentifier = -1;
    IosDevice::ConstPtr device;
    FilePath bundlePath;
    QString bundleIdentifier;
    QStringList arguments;
};

static GroupItem initSetup(RunControl *runControl, const Storage<AppInfo> &appInfo)
{
    const auto onSetup = [runControl, appInfo] {
        const IosDeviceTypeAspect::Data *data = runControl->aspectData<IosDeviceTypeAspect>();
        if (!data)
            return false;

        appInfo->bundlePath = data->bundleDirectory;
        appInfo->bundleIdentifier = getBundleIdentifier(appInfo->bundlePath);
        if (appInfo->bundleIdentifier.isEmpty()) {
            runControl->postMessage(Tr::tr("Failed to determine bundle identifier."), ErrorMessageFormat);
            return false;
        }
        runControl->postMessage(Tr::tr("Running \"%1\" on %2...")
            .arg(appInfo->bundlePath.toUserOutput(), runControl->device()->displayName()),
            NormalMessageFormat);
        appInfo->device = std::dynamic_pointer_cast<const IosDevice>(runControl->device());
        appInfo->arguments = ProcessArgs::splitArgs(runControl->commandLine().arguments(), OsTypeMac);
        return true;
    };
    return Sync(onSetup);
}

static GroupItem findApp(RunControl *runControl, const Storage<AppInfo> &appInfo)
{
    const auto onSetup = [appInfo](Process &process) {
        if (!appInfo->device)
            return SetupResult::StopWithSuccess; // don't block the following tasks
        process.setCommand({FilePath::fromString("/usr/bin/xcrun"),
                            {"devicectl",
                             "device",
                             "info",
                             "apps",
                             "--device",
                             appInfo->device->uniqueInternalDeviceId(),
                             "--quiet",
                             "--json-output",
                             "-"}});
        return SetupResult::Continue;
    };
    const auto onDone = [runControl, appInfo](const Process &process) {
        if (process.error() != QProcess::UnknownError) {
            runControl->postMessage(Tr::tr("Failed to run devicectl: %1.").arg(process.errorString()),
                                    ErrorMessageFormat);
            return DoneResult::Error;
        }
        const Result<QUrl> pathOnDevice = parseAppInfo(process.rawStdOut(), appInfo->bundleIdentifier);
        if (pathOnDevice) {
            appInfo->pathOnDevice = *pathOnDevice;
            return DoneResult::Success;
        }
        runControl->postMessage(pathOnDevice.error(), ErrorMessageFormat);
        return DoneResult::Error;
    };
    return ProcessTask(onSetup, onDone);
}

static GroupItem findProcess(RunControl *runControl, const Storage<AppInfo> &appInfo)
{
    const auto onSetup = [appInfo](Process &process) {
        if (!appInfo->device || appInfo->pathOnDevice.isEmpty())
            return SetupResult::StopWithSuccess; // don't block the following tasks
        process.setCommand(
            {FilePath::fromString("/usr/bin/xcrun"),
             {"devicectl",
              "device",
              "info",
              "processes",
              "--device",
              appInfo->device->uniqueInternalDeviceId(),
              "--quiet",
              "--json-output",
              "-",
              "--filter",
              QLatin1String("executable.path BEGINSWITH '%1'").arg(appInfo->pathOnDevice.path())}});
        return SetupResult::Continue;
    };
    const auto onDone = [runControl, appInfo](const Process &process) {
        const Utils::Result<qint64> pid = parseProcessIdentifier(process.rawStdOut());
        if (pid) {
            appInfo->processIdentifier = *pid;
            return DoneResult::Success;
        }
        runControl->postMessage(pid.error(), ErrorMessageFormat);
        return DoneResult::Error;
    };
    return ProcessTask(onSetup, onDone);
}

static GroupItem killProcess(const Storage<AppInfo> &appInfo)
{
    const auto onSetup = [appInfo](Process &process) {
        if (!appInfo->device || appInfo->processIdentifier < 0)
            return SetupResult::StopWithSuccess; // don't block the following tasks
        process.setCommand({FilePath::fromString("/usr/bin/xcrun"),
                            {"devicectl",
                             "device",
                             "process",
                             "signal",
                             "--device",
                             appInfo->device->uniqueInternalDeviceId(),
                             "--quiet",
                             "--json-output",
                             "-",
                             "--signal",
                             "SIGKILL",
                             "--pid",
                             QString::number(appInfo->processIdentifier)}});
        return SetupResult::Continue;
    };
    return ProcessTask(onSetup, DoneResult::Success); // we tried our best and don't care at this point
}

static Group deviceCtlKicker(const SingleBarrier &barrier, RunControl *runControl,
                             const Storage<AppInfo> &appInfo,
                             const Storage<TemporaryFile> tempFileStorage, bool startStopped)
{
    const auto launchApp = [runControl, appInfo, tempFileStorage, startStopped](const SingleBarrier &barrier) {
        const auto onSetup = [runControl, appInfo, tempFileStorage, startStopped, barrier](Process &process) {
            const QStringList startStoppedArg = startStopped ? QStringList("--start-stopped")
                                                             : QStringList();
            const QStringList args = QStringList(
                                         {"devicectl",
                                          "device",
                                          "process",
                                          "launch",
                                          "--device",
                                          appInfo->device->uniqueInternalDeviceId(),
                                          "--quiet",
                                          "--json-output",
                                          tempFileStorage->fileName()})
                                     + startStoppedArg
                                     + QStringList({"--console", appInfo->bundleIdentifier})
                                     + appInfo->arguments;
            process.setCommand({FilePath::fromString("/usr/bin/xcrun"), args});
            QObject::connect(&process, &Process::started, barrier->barrier(), &Barrier::advance);

            QObject::connect(&process, &Process::readyReadStandardError, runControl,
                             [runControl, process = &process] {
                runControl->postMessage(process->readAllStandardError(), StdErrFormat, false);
            });
            QObject::connect(&process, &Process::readyReadStandardOutput, runControl,
                             [runControl, process = &process] {
                runControl->postMessage(process->readAllStandardOutput(), StdOutFormat, false);
            });
            QObject::connect(runStorage().activeStorage(), &RunInterface::canceled,
                             &process, &Process::stop);
        };
        const auto onDone = [runControl, appInfo](const Process &process) {
            if (process.error() != QProcess::UnknownError) {
                runControl->postMessage(Tr::tr("Failed to run devicectl: %1.").arg(process.errorString()),
                                        ErrorMessageFormat);
            } else {
                runControl->postMessage(Tr::tr("\"%1\" exited.").arg(appInfo->bundlePath.toUserOutput()),
                                        NormalMessageFormat);
            }
        };

        return ProcessTask(onSetup, onDone);
    };

    const auto onDone = [runControl, appInfo, barrier](DoneWith result) {
        if (result == DoneWith::Success) {
            runControl->setAttachPid(ProcessHandle(appInfo->processIdentifier));
            barrier->barrier()->advance();
        } else {
            runControl->postMessage(Tr::tr("Failed to retrieve process ID."), ErrorMessageFormat);
        }
    };

    return {
        When (launchApp) >> Do {
            findApp(runControl, appInfo),
            findProcess(runControl, appInfo),
            onGroupDone(onDone)
        }
    };
}

static Group killApp(RunControl *runControl, const Storage<AppInfo> &appInfo)
{
    // If the app is already running, we should first kill it, then launch again.
    // Usually deployment already kills the running app, but we support running without
    // deployment. Restarting is then e.g. needed if the app arguments changed.
    // Unfortunately the information about running processes only includes the path
    // on device and processIdentifier.
    // So we find out if the app is installed, and its path on device.
    // Check if a process is running for that path, and get its processIdentifier.
    // Try to kill that.
    // Then launch the app (again).
    return Group {
        findApp(runControl, appInfo),
        findProcess(runControl, appInfo),
        killProcess(appInfo)
    };
}

static Group deviceCtlKicker(const SingleBarrier &barrier, RunControl *runControl, bool startStopped)
{
    const Storage<AppInfo> appInfo;
    const Storage<TemporaryFile> tempFileStorage{QString("devicectl")};

    const auto onSetup = [runControl, appInfo, tempFileStorage] {
        if (!appInfo->device) {
            runControl->postMessage(Tr::tr("Running failed. No iOS device found."),
                                    ErrorMessageFormat);
            return false;
        }
        if (!tempFileStorage->open() || tempFileStorage->fileName().isEmpty()) {
            runControl->postMessage(Tr::tr("Running failed. Failed to create the temporary output file."),
                                    ErrorMessageFormat);
            return false;
        }
        return true;
    };

    return {
        appInfo,
        tempFileStorage,
        Group {
            initSetup(runControl, appInfo),
            Sync(onSetup),
            killApp(runControl, appInfo),
        }.withCancel(canceler()),
        deviceCtlKicker(barrier, runControl, appInfo, tempFileStorage, startStopped)
    };
}

static Group deviceCtlRecipe(RunControl *runControl, bool startStopped)
{
    const auto kicker = [runControl, startStopped](const SingleBarrier &barrier) {
        return deviceCtlKicker(barrier, runControl, startStopped);
    };
    return When (kicker) >> Do {
        Sync([] { emit runStorage()->started(); })
    };
}

static Group deviceCtlPollingTask(RunControl *runControl, const Storage<AppInfo> &appInfo)
{
    const Storage<qint64> pidStorage;

    const auto onLaunchSetup = [appInfo](Process &process) {
        process.setCommand({FilePath::fromString("/usr/bin/xcrun"),
                            {"devicectl",
                             "device",
                             "process",
                             "launch",
                             "--device",
                             appInfo->device->uniqueInternalDeviceId(),
                             "--quiet",
                             "--json-output",
                             "-",
                             appInfo->bundleIdentifier,
                             appInfo->arguments}});
    };
    const auto onLaunchDone = [runControl, pidStorage](const Process &process, DoneWith result) {
        if (result == DoneWith::Cancel) {
            runControl->postMessage(Tr::tr("Running canceled."), ErrorMessageFormat);
            return DoneResult::Error;
        }
        if (process.error() != QProcess::UnknownError) {
            runControl->postMessage(Tr::tr("Failed to run devicectl: %1.").arg(process.errorString()),
                                    ErrorMessageFormat);
            return DoneResult::Error;
        }
        const Result<qint64> pid = parseLaunchResult(process.rawStdOut());
        if (pid) {
            *pidStorage = *pid;
            runControl->setAttachPid(ProcessHandle(*pid));
            emit runStorage()->started();
            return DoneResult::Success;
        }
        runControl->postMessage(pid.error(), ErrorMessageFormat);
        return DoneResult::Error;
    };

    const auto onPollSetup = [appInfo, pidStorage](Process &process) {
        process.setCommand(
            {FilePath::fromString("/usr/bin/xcrun"),
             {"devicectl",
              "device",
              "info",
              "processes",
              "--device",
              appInfo->device->uniqueInternalDeviceId(),
              "--quiet",
              "--json-output",
              "-",
              "--filter",
              QLatin1String("processIdentifier == %1").arg(QString::number(*pidStorage))}});
    };
    const auto onPollDone = [runControl, appInfo](const Process &process, DoneWith result) {
        if (result == DoneWith::Cancel)
            return DoneResult::Error;
        const Result<QJsonValue> resultValue = parseDevicectlResult(process.rawStdOut());
        if (!resultValue || (*resultValue)["runningProcesses"].toArray().size() < 1) {
            // no process with processIdentifier found, or some error occurred, device disconnected
            // or such, assume "stopped"
            runControl->postMessage(Tr::tr("\"%1\" exited.").arg(appInfo->bundlePath.toUserOutput()),
                                    NormalMessageFormat);
            return DoneResult::Error;
        }
        return DoneResult::Success;
    };

    const auto onStopSetup = [appInfo, pidStorage](Process &process) {
        process.setCommand({FilePath::fromString("/usr/bin/xcrun"),
                            {"devicectl",
                             "device",
                             "process",
                             "signal",
                             "--device",
                             appInfo->device->uniqueInternalDeviceId(),
                             "--quiet",
                             "--json-output",
                             "-",
                             "--signal",
                             "SIGKILL",
                             "--pid",
                             QString::number(*pidStorage)}});
    };
    const auto onStopDone = [runControl, appInfo](const Process &process) {
        if (process.error() != QProcess::UnknownError) {
            runControl->postMessage(Tr::tr("Failed to run devicectl: %1.").arg(process.errorString()),
                                    ErrorMessageFormat);
            return DoneResult::Error;
        }
        const Result<QJsonValue> resultValue = parseDevicectlResult(process.rawStdOut());
        if (!resultValue) {
            runControl->postMessage(resultValue.error(), ErrorMessageFormat);
            return DoneResult::Error;
        }
        runControl->postMessage(Tr::tr("\"%1\" exited.").arg(appInfo->bundlePath.toUserOutput()),
                      NormalMessageFormat);
        return DoneResult::Success;
    };

    using namespace std::chrono_literals;

    return {
        pidStorage,
        ProcessTask(onLaunchSetup, onLaunchDone).withCancel(canceler()),
        Group {
            Forever {
                timeoutTask(500ms, DoneResult::Success),
                ProcessTask(onPollSetup, onPollDone)
            }.withCancel(canceler(), {
                ProcessTask(onStopSetup, onStopDone)
            }),
        },
    };
}

static Group deviceCtlPollingRecipe(RunControl *runControl)
{
    const Storage<AppInfo> appInfo;

    const auto onSetup = [runControl, appInfo] {
        if (!appInfo->device) {
            runControl->postMessage(Tr::tr("Running failed. No iOS device found."),
                                    ErrorMessageFormat);
            return false;
        }
        return true;
    };

    return {
        appInfo,
        Group {
            initSetup(runControl, appInfo),
            Sync(onSetup),
            killApp(runControl, appInfo),
        }.withCancel(canceler()),
        deviceCtlPollingTask(runControl, appInfo)
    };
}

struct DebugInfo
{
    QmlDebugServicesPreset qmlDebugServices = NoQmlDebugServices;
    bool cppDebug = false;
};

static void handleIosToolErrorMessage(RunControl *runControl, const QString &message)
{
    QString res(message);
    const QString lockedErr = "Unexpected reply: ELocked (454c6f636b6564) vs OK (4f4b)";
    if (message.contains("AMDeviceStartService returned -402653150")) {
        TaskHub::addTask(DeploymentTask(
            Task::Warning,
            Tr::tr("Run failed. "
                   "The settings in the Organizer window of Xcode might be incorrect.")));
    } else if (res.contains(lockedErr)) {
        QString message = Tr::tr("The device is locked, please unlock.");
        TaskHub::addTask(DeploymentTask(Task::Error, message));
        res.replace(lockedErr, message);
    }
    runControl->postMessage(res, StdErrFormat);
}

static void handleIosToolStartedOnDevice(
    Barrier *barrier,
    RunControl *runControl,
    const DebugInfo &debugInfo,
    IosToolHandler *handler,
    Port gdbPort,
    Port qmlPort)
{
    QUrl debugChannel;
    debugChannel.setScheme("connect");
    debugChannel.setHost("localhost");
    debugChannel.setPort(gdbPort.number());
    runControl->setDebugChannel(debugChannel);
    // The run control so far knows about the port on the device side,
    // but the QML Profiler has to actually connect to a corresponding
    // local port. That port is reported here, so we need to adapt the runControl's
    // "qmlChannel", so the QmlProfilerRunner uses the right port.
    QUrl qmlChannel = runControl->qmlChannel();
    const int qmlPortOnDevice = qmlChannel.port();
    qmlChannel.setPort(qmlPort.number());
    runControl->setQmlChannel(qmlChannel);

    if (debugInfo.cppDebug) {
        if (!gdbPort.isValid()) {
            runControl
                ->postMessage(Tr::tr("Failed to get a local debugger port."), ErrorMessageFormat);
            handler->stop();
            return;
        }
        runControl->postMessage(
            Tr::tr("Listening for debugger on local port %1.").arg(gdbPort.number()),
            LogMessageFormat);
    }
    if (debugInfo.qmlDebugServices != NoQmlDebugServices) {
        if (!qmlPort.isValid()) {
            runControl->postMessage(
                Tr::tr("Failed to get a local debugger port for QML."), ErrorMessageFormat);
            handler->stop();
            return;
        }
        runControl->postMessage(
            Tr::tr("Listening for QML debugger on local port %1 (port %2 on the device).")
                .arg(qmlPort.number())
                .arg(qmlPortOnDevice),
            LogMessageFormat);
    }
    barrier->advance();
}

static void handleIosToolStartedOnSimulator(
    Barrier *barrier,
    RunControl *runControl,
    const DebugInfo &debugInfo,
    IosToolHandler *handler,
    qint64 pid)
{
    if (pid <= 0) {
        runControl->postMessage(Tr::tr("Could not get inferior PID."), ErrorMessageFormat);
        handler->stop();
        return;
    }
    runControl->setAttachPid(ProcessHandle(pid));
    if (debugInfo.qmlDebugServices != NoQmlDebugServices && runControl->qmlChannel().port() == -1) {
        runControl->postMessage(
            Tr::tr("Could not get necessary ports for the QML debugger connection."),
            ErrorMessageFormat);
        handler->stop();
        return;
    }
    barrier->advance();
}

static Group iosToolKicker(const SingleBarrier &barrier, RunControl *runControl, const DebugInfo &debugInfo)
{
    stopRunningRunControl(runControl);
    const IosDeviceTypeAspect::Data *data = runControl->aspectData<IosDeviceTypeAspect>();
    QTC_ASSERT(data, return {});

    const FilePath bundleDir = data->bundleDirectory;
    const IosDeviceType deviceType = data->deviceType;
    const IDeviceConstPtr device = RunDeviceKitAspect::device(runControl->kit());

    const auto onSetup = [bundleDir] {
        if (bundleDir.exists())
            return SetupResult::Continue;

        TaskHub::addTask(DeploymentTask(Task::Warning, Tr::tr("Could not find %1.")
                                                           .arg(bundleDir.toUserOutput())));
        return SetupResult::StopWithError;
    };

    const auto onIosToolSetup = [runControl, debugInfo, bundleDir, deviceType, device,
                                 barrier](IosToolRunner &runner) {
        runner.setDeviceType(deviceType);
        RunInterface *iface = runStorage().activeStorage();
        runner.setStartHandler([runControl, debugInfo, bundleDir, device, iface,
                                barrier = barrier->barrier()](IosToolHandler *handler) {
            const auto messageHandler = [runControl](const QString &message) {
                runControl->postMessage(message, StdOutFormat);
            };

            QObject::connect(handler, &IosToolHandler::appOutput, runControl, messageHandler);
            QObject::connect(handler, &IosToolHandler::message, runControl, messageHandler);
            QObject::connect(
                handler, &IosToolHandler::errorMsg, runControl, [runControl](const QString &message) {
                    handleIosToolErrorMessage(runControl, message);
                });
            QObject::connect(
                handler,
                &IosToolHandler::gotServerPorts,
                runControl,
                [barrier, runControl, debugInfo, handler](Port gdbPort, Port qmlPort) {
                    handleIosToolStartedOnDevice(
                        barrier, runControl, debugInfo, handler, gdbPort, qmlPort);
                });
            QObject::connect(
                handler,
                &IosToolHandler::gotInferiorPid,
                runControl,
                [barrier, runControl, debugInfo, handler](qint64 pid) {
                    handleIosToolStartedOnSimulator(barrier, runControl, debugInfo, handler, pid);
                });
            QObject::connect(iface, &RunInterface::canceled, handler, [handler] {
                if (handler->isRunning())
                    handler->stop();
            });

            const CommandLine command = runControl->commandLine();
            QStringList args = ProcessArgs::splitArgs(command.arguments(), OsTypeMac);
            const Port portOnDevice = Port(runControl->qmlChannel().port());
            if (portOnDevice.isValid()) {
                QUrl qmlServer;
                qmlServer.setPort(portOnDevice.number());
                args.append(qmlDebugTcpArguments(debugInfo.qmlDebugServices, qmlServer));
            }

            runControl->postMessage(Tr::tr("Starting remote process."), NormalMessageFormat);
            QString deviceId;
            if (IosDevice::ConstPtr dev = std::dynamic_pointer_cast<const IosDevice>(device))
                deviceId = dev->uniqueDeviceID();
            const IosToolHandler::RunKind runKind = debugInfo.cppDebug ? IosToolHandler::DebugRun
                                                                       : IosToolHandler::NormalRun;
            handler->requestRunApp(bundleDir, args, runKind, deviceId);
        });
    };
    const auto onIosToolDone = [runControl](DoneWith result) {
        if (result == DoneWith::Success)
            runControl->postMessage(Tr::tr("Run ended."), NormalMessageFormat);
        else
            runControl->postMessage(Tr::tr("Run ended with error."), ErrorMessageFormat);
    };

    return {
        onGroupSetup(onSetup),
        IosToolTask(onIosToolSetup, onIosToolDone)
    };
}

static Group iosToolRecipe(RunControl *runControl, const DebugInfo &debugInfo = {})
{
    const auto kicker = [runControl, debugInfo](const SingleBarrier &barrier) {
        return iosToolKicker(barrier, runControl, debugInfo);
    };
    return When (kicker) >> Do {
        Sync([] { emit runStorage()->started(); })
    };
}

static Result<FilePath> findDeviceSdk(IosDevice::ConstPtr dev)
{
    const QString osVersion = dev->osVersion();
    const QString productType = dev->productType();
    const QString cpuArchitecture = dev->cpuArchitecture();
    const FilePath home = FilePath::fromString(QDir::homePath());
    const FilePaths symbolsPathCandidates
        = {home / "Library/Developer/Xcode/iOS DeviceSupport" / (productType + " " + osVersion)
               / "Symbols",
           home / "Library/Developer/Xcode/iOS DeviceSupport" / (osVersion + " " + cpuArchitecture)
               / "Symbols",
           home / "Library/Developer/Xcode/iOS DeviceSupport" / osVersion / "Symbols",
           IosConfigurations::developerPath() / "Platforms/iPhoneOS.platform/DeviceSupport"
               / osVersion / "Symbols"};
    const FilePath deviceSdk = Utils::findOrDefault(symbolsPathCandidates, &FilePath::isDir);
    if (deviceSdk.isEmpty()) {
        return ResultError(
            Tr::tr("Could not find device specific debug symbols at %1. "
                   "Debugging initialization will be slow until you open the Organizer window of "
                   "Xcode with the device connected to have the symbols generated.")
                .arg(symbolsPathCandidates.constFirst().toUserOutput()));
    }
    return deviceSdk;
}

// Factories

IosRunWorkerFactory::IosRunWorkerFactory()
{
    setProducer([](RunControl *runControl) -> RunWorker * {
        IosDevice::ConstPtr iosdevice = std::dynamic_pointer_cast<const IosDevice>(runControl->device());
        if (iosdevice && iosdevice->handler() == IosDevice::Handler::DeviceCtl) {
            if (IosDeviceManager::isDeviceCtlOutputSupported())
                return new RunWorker(runControl, deviceCtlRecipe(runControl, /*startStopped=*/ false));
            // TODO Remove the polling runner when we decide not to support iOS 17+ devices
            // with Xcode < 16 at all
            return new RunWorker(runControl, deviceCtlPollingRecipe(runControl));
        }
        runControl->setIcon(Icons::RUN_SMALL_TOOLBAR);
        runControl->setDisplayName(QString("Run on %1")
                                       .arg(iosdevice ? iosdevice->displayName() : QString()));
        return new RunWorker(runControl, iosToolRecipe(runControl));
    });
    addSupportedRunMode(ProjectExplorer::Constants::NORMAL_RUN_MODE);
    addSupportedRunConfig(Constants::IOS_RUNCONFIG_ID);
}

static void parametersModifier(RunControl *runControl, DebuggerRunParameters &rp)
{
    const bool cppDebug = rp.isCppDebugging();
    const bool qmlDebug = rp.isQmlDebugging();
    if (cppDebug) {
        const IosDeviceTypeAspect::Data *data = runControl->aspectData<IosDeviceTypeAspect>();
        rp.setInferiorExecutable(data->localExecutable);
        rp.setRemoteChannel(runControl->debugChannel());

        QString bundlePath = data->bundleDirectory.toUrlishString();
        bundlePath.chop(4);
        const FilePath dsymPath = FilePath::fromString(bundlePath.append(".dSYM"));
        if (dsymPath.exists()
            && dsymPath.lastModified() < data->localExecutable.lastModified()) {
            TaskHub::addTask(DeploymentTask(Task::Warning,
                                            Tr::tr("The dSYM %1 seems to be outdated, it might confuse the debugger.")
                                                .arg(dsymPath.toUserOutput())));
        }
    }

    if (qmlDebug) {
        QTcpServer server;
        const bool isListening = server.listen(QHostAddress::LocalHost)
                                 || server.listen(QHostAddress::LocalHostIPv6);
        QTC_ASSERT(isListening, return);
        QUrl qmlServer;
        qmlServer.setHost(server.serverAddress().toString());
        if (!cppDebug)
            rp.setStartMode(AttachToRemoteServer);
        qmlServer.setPort(runControl->qmlChannel().port());
        rp.setQmlServer(qmlServer);
    }
}

static QString msgOnlyCppDebuggingSupported()
{
    return Tr::tr("Only C++ debugging is supported for devices with iOS 17 and later.");
};

static RunWorker *createDebugWorker(RunControl *runControl)
{
    IosDevice::ConstPtr dev = std::dynamic_pointer_cast<const IosDevice>(runControl->device());
    const bool isIosDeviceType = runControl->device()->type() == Ios::Constants::IOS_DEVICE_TYPE;
    const bool isIosDeviceInstance = bool(dev);
    // type info and device class must match
    const bool isOK = isIosDeviceInstance == isIosDeviceType;
    const bool isIosRunner = !isIosDeviceInstance /*== simulator */ || dev->handler() == IosDevice::Handler::IosTool;
    DebuggerRunParameters rp = DebuggerRunParameters::fromRunControl(runControl);
    // TODO cannot use setupPortsGatherer(), because that also requests
    // the "debugChannel", which then results in runControl trying to retrieve ports&URL for that
    // via IDevice, which doesn't really work with the iOS setup, and also completely changes
    // how the debuggerRecipe() works, breaking debugging on iOS <= 16 devices.
    if (rp.isQmlDebugging())
        runControl->requestQmlChannel();

    const IosDeviceTypeAspect::Data *data = runControl->aspectData<IosDeviceTypeAspect>();
    if (data)
        rp.setDisplayName(data->applicationName);
    rp.setContinueAfterAttach(true);

    if (isIosDeviceInstance) {
        if (dev->handler() == IosDevice::Handler::DeviceCtl) {
            QTC_CHECK(IosDeviceManager::isDeviceCtlDebugSupported());
            rp.setStartMode(AttachToIosDevice);
            rp.setDeviceUuid(dev->uniqueInternalDeviceId());
        } else {
            rp.setStartMode(AttachToRemoteProcess);
        }
        rp.setLldbPlatform("remote-ios");
        const Result<FilePath> deviceSdk = findDeviceSdk(dev);

        if (!deviceSdk)
            TaskHub::addTask(DeploymentTask(Task::Warning, deviceSdk.error()));
        else
            rp.setDeviceSymbolsRoot(deviceSdk->path());
    } else {
        rp.setStartMode(AttachToLocalProcess);
        rp.setLldbPlatform("ios-simulator");
    }

    BarrierKickerGetter kicker;
    if (isIosRunner) {
        const DebugInfo debugInfo{rp.isQmlDebugging() ? QmlDebuggerServices : NoQmlDebugServices,
                                  rp.isCppDebugging()};
        kicker = [runControl, debugInfo](const SingleBarrier &barrier) {
            return iosToolKicker(barrier, runControl, debugInfo);
        };
    } else {
        if (data)
            rp.setInferiorExecutable(data->localExecutable);
        const bool warnAboutQml = rp.isQmlDebugging();
        rp.setQmlDebugging(false);
        kicker = [runControl, warnAboutDebug = rp.isCppDebugging(), warnAboutQml](const SingleBarrier &barrier) {
            const auto onSetup = [runControl, warnAboutDebug, warnAboutQml] {
                QTC_ASSERT(warnAboutDebug,
                           runControl->postMessage(msgOnlyCppDebuggingSupported(), ErrorMessageFormat);
                           return SetupResult::StopWithError);
                if (warnAboutQml)
                    runControl->postMessage(msgOnlyCppDebuggingSupported(), LogMessageFormat);
                return SetupResult::Continue;
            };
            return Group {
                onGroupSetup(onSetup),
                deviceCtlKicker(barrier, runControl, /*startStopped=*/ true)
            };
        };
    }

    // TODO cannot use setupPortsGatherer(), because that also requests
    // the "debugChannel", which then results in runControl trying to retrieve ports&URL for that
    // via IDevice, which doesn't really work with the iOS setup, and also completely changes
    // how the debuggerRecipe() works, breaking debugging on iOS <= 16 devices.
    if (rp.isQmlDebugging())
        runControl->requestQmlChannel();

    const auto onSetup = [runControl, isOK] {
        QTC_ASSERT(isOK,
                   runControl->postMessage(Tr::tr("Internal error."), ErrorMessageFormat);
                   return SetupResult::StopWithError);
        QTC_ASSERT(runControl->aspectData<IosDeviceTypeAspect>(),
                   runControl->postMessage("Broken IosDeviceTypeAspect setup.", ErrorMessageFormat);
                   return SetupResult::StopWithError);
        return SetupResult::Continue;
    };

    const auto modifier = [runControl, isIosRunner](DebuggerRunParameters &rp) {
        if (isIosRunner)
            parametersModifier(runControl, rp);
    };

    const Group recipe {
        onGroupSetup(onSetup),
        When (kicker) >> Do {
            debuggerRecipe(runControl, rp, modifier)
        }
    };

    return new RunWorker(runControl, recipe);
}

IosDebugWorkerFactory::IosDebugWorkerFactory()
{
    setProducer([](RunControl *runControl) { return createDebugWorker(runControl); });
    addSupportedRunMode(ProjectExplorer::Constants::DEBUG_RUN_MODE);
    addSupportedRunConfig(Constants::IOS_RUNCONFIG_ID);
}

IosQmlProfilerWorkerFactory::IosQmlProfilerWorkerFactory()
{
    setProducer([](RunControl *runControl) {
        auto runner = new RunWorker(runControl, iosToolRecipe(runControl, {QmlProfilerServices}));
        runControl->requestQmlChannel();
        auto profiler = runControl->createWorker(ProjectExplorer::Constants::QML_PROFILER_RUNNER);
        profiler->addStartDependency(runner);
        return profiler;
    });
    addSupportedRunMode(ProjectExplorer::Constants::QML_PROFILER_RUN_MODE);
    addSupportedRunConfig(Constants::IOS_RUNCONFIG_ID);
}

} // Ios::Internal
