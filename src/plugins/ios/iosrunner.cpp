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

static Group deviceCtlTask(RunControl *runControl, const Storage<AppInfo> &appInfo,
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

    const auto onDone = [runControl, appInfo](DoneWith result) {
        if (result == DoneWith::Success) {
            runControl->setAttachPid(ProcessHandle(appInfo->processIdentifier));
            emit runStorage()->started();
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

static Group deviceCtlRecipe(RunControl *runControl, bool startStopped)
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
            findApp(runControl, appInfo),
            findProcess(runControl, appInfo),
            killProcess(appInfo)
        }.withCancel(canceler()),
        deviceCtlTask(runControl, appInfo, tempFileStorage, startStopped)
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
            findApp(runControl, appInfo),
            findProcess(runControl, appInfo),
            killProcess(appInfo)
        }.withCancel(canceler()),
        deviceCtlPollingTask(runControl, appInfo)
    };
}

struct DebugInfo
{
    QmlDebugServicesPreset qmlDebugServices = NoQmlDebugServices;
    bool cppDebug = false;
};

class IosRunner : public RunWorker
{
public:
    IosRunner(RunControl *runControl, const DebugInfo &debugInfo = {});
    ~IosRunner() override;

    void start() override;
    void stop() final;

private:
    void handleGotServerPorts(Ios::IosToolHandler *handler, const FilePath &bundlePath,
                              const QString &deviceId, Port gdbPort, Port qmlPort);
    void handleGotInferiorPid(Ios::IosToolHandler *handler, const FilePath &bundlePath,
                              const QString &deviceId, qint64 pid);
    void handleMessage(const QString &msg);
    void handleErrorMsg(const QString &msg);
    void handleToolExited(int code);

    IosToolHandler *m_toolHandler = nullptr;
    FilePath m_bundleDir;
    IDeviceConstPtr m_device;
    IosDeviceType m_deviceType;
    DebugInfo m_debugInfo;
    bool m_cleanExit = false;
};

IosRunner::IosRunner(RunControl *runControl, const DebugInfo &debugInfo)
    : RunWorker(runControl)
    , m_debugInfo(debugInfo)
{
    setId("IosRunner");
    stopRunningRunControl(runControl);
    const IosDeviceTypeAspect::Data *data = runControl->aspectData<IosDeviceTypeAspect>();
    QTC_ASSERT(data, return);
    m_bundleDir = data->bundleDirectory;
    m_device = RunDeviceKitAspect::device(runControl->kit());
    m_deviceType = data->deviceType;
}

IosRunner::~IosRunner()
{
    stop();
}

void IosRunner::start()
{
    if (m_toolHandler && m_toolHandler->isRunning())
        m_toolHandler->stop();

    m_cleanExit = false;
    if (!m_bundleDir.exists()) {
        TaskHub::addTask(DeploymentTask(Task::Warning,
            Tr::tr("Could not find %1.").arg(m_bundleDir.toUserOutput())));
        reportFailure();
        return;
    }

    m_toolHandler = new IosToolHandler(m_deviceType, this);
    connect(m_toolHandler, &IosToolHandler::appOutput, this, &IosRunner::handleMessage);
    connect(m_toolHandler, &IosToolHandler::message, this, &IosRunner::handleMessage);
    connect(m_toolHandler, &IosToolHandler::errorMsg, this, &IosRunner::handleErrorMsg);
    connect(m_toolHandler, &IosToolHandler::gotServerPorts, this, &IosRunner::handleGotServerPorts);
    connect(m_toolHandler, &IosToolHandler::gotInferiorPid, this, &IosRunner::handleGotInferiorPid);
    connect(m_toolHandler, &IosToolHandler::toolExited, this, &IosRunner::handleToolExited);
    connect(m_toolHandler, &IosToolHandler::finished, this, [this, handler = m_toolHandler] {
        if (m_toolHandler == handler) {
            if (m_cleanExit)
                appendMessage(Tr::tr("Run ended."), NormalMessageFormat);
            else
                appendMessage(Tr::tr("Run ended with error."), ErrorMessageFormat);
            m_toolHandler = nullptr;
        }
        handler->deleteLater();
        reportStopped();

    });

    const CommandLine command = runControl()->commandLine();
    QStringList args = ProcessArgs::splitArgs(command.arguments(), OsTypeMac);
    const Port portOnDevice = Port(runControl()->qmlChannel().port());
    if (portOnDevice.isValid()) {
        QUrl qmlServer;
        qmlServer.setPort(portOnDevice.number());
        args.append(qmlDebugTcpArguments(m_debugInfo.qmlDebugServices, qmlServer));
    }

    appendMessage(Tr::tr("Starting remote process."), NormalMessageFormat);
    QString deviceId;
    if (IosDevice::ConstPtr dev = std::dynamic_pointer_cast<const IosDevice>(m_device))
        deviceId = dev->uniqueDeviceID();
    const IosToolHandler::RunKind runKind = m_debugInfo.cppDebug ? IosToolHandler::DebugRun : IosToolHandler::NormalRun;
    m_toolHandler->requestRunApp(m_bundleDir, args, runKind, deviceId);
}

void IosRunner::stop()
{
    if (m_toolHandler && m_toolHandler->isRunning())
        m_toolHandler->stop();
}

void IosRunner::handleGotServerPorts(IosToolHandler *handler, const FilePath &bundlePath,
                                     const QString &deviceId, Port gdbPort,
                                     Port qmlPort)
{
    // Called when debugging on Device.
    Q_UNUSED(bundlePath)
    Q_UNUSED(deviceId)

    if (m_toolHandler != handler)
        return;

    // TODO: Convert to portsgatherer.
    QUrl debugChannel;
    debugChannel.setScheme("connect");
    debugChannel.setHost("localhost");
    debugChannel.setPort(gdbPort.number());
    runControl()->setDebugChannel(debugChannel);
    // The run control so far knows about the port on the device side,
    // but the QML Profiler has to actually connect to a corresponding
    // local port. That port is reported here, so we need to adapt the runControl's
    // "qmlChannel", so the QmlProfilerRunner uses the right port.
    QUrl qmlChannel = runControl()->qmlChannel();
    const int qmlPortOnDevice = qmlChannel.port();
    qmlChannel.setPort(qmlPort.number());
    runControl()->setQmlChannel(qmlChannel);

    if (m_debugInfo.cppDebug) {
        if (!gdbPort.isValid()) {
            reportFailure(Tr::tr("Failed to get a local debugger port."));
            return;
        }
        appendMessage(
            Tr::tr("Listening for debugger on local port %1.").arg(gdbPort.number()),
            LogMessageFormat);
    }
    if (m_debugInfo.qmlDebugServices != NoQmlDebugServices) {
        if (!qmlPort.isValid()) {
            reportFailure(Tr::tr("Failed to get a local debugger port."));
            return;
        }
        appendMessage(
            Tr::tr("Listening for QML debugger on local port %1 (port %2 on the device).")
                .arg(qmlPort.number()).arg(qmlPortOnDevice),
            LogMessageFormat);
    }

    reportStarted();
}

void IosRunner::handleGotInferiorPid(IosToolHandler *handler, const FilePath &bundlePath,
                                     const QString &deviceId, qint64 pid)
{
    // Called when debugging on Simulator.
    Q_UNUSED(bundlePath)
    Q_UNUSED(deviceId)

    if (m_toolHandler != handler)
        return;

    if (pid <= 0) {
        reportFailure(Tr::tr("Could not get inferior PID."));
        return;
    }
    runControl()->setAttachPid(ProcessHandle(pid));

    if (m_debugInfo.qmlDebugServices != NoQmlDebugServices && runControl()->qmlChannel().port() == -1)
        reportFailure(Tr::tr("Could not get necessary ports for the debugger connection."));
    else
        reportStarted();
}

void IosRunner::handleMessage(const QString &msg)
{
    appendMessage(msg, StdOutFormat);
}

void IosRunner::handleErrorMsg(const QString &msg)
{
    QString res(msg);
    QString lockedErr ="Unexpected reply: ELocked (454c6f636b6564) vs OK (4f4b)";
    if (msg.contains("AMDeviceStartService returned -402653150")) {
        TaskHub::addTask(DeploymentTask(Task::Warning, Tr::tr("Run failed. "
           "The settings in the Organizer window of Xcode might be incorrect.")));
    } else if (res.contains(lockedErr)) {
        QString message = Tr::tr("The device is locked, please unlock.");
        TaskHub::addTask(DeploymentTask(Task::Error, message));
        res.replace(lockedErr, message);
    }
    appendMessage(res, StdErrFormat);
}

void IosRunner::handleToolExited(int code)
{
    m_cleanExit = (code == 0);
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
                return new RecipeRunner(runControl, deviceCtlRecipe(runControl, /*startStopped=*/ false));
            // TODO Remove the polling runner when we decide not to support iOS 17+ devices
            // with Xcode < 16 at all
            return new RecipeRunner(runControl, deviceCtlPollingRecipe(runControl));
        }
        runControl->setIcon(Icons::RUN_SMALL_TOOLBAR);
        runControl->setDisplayName(QString("Run on %1")
                                       .arg(iosdevice ? iosdevice->displayName() : QString()));
        return new IosRunner(runControl);
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

static RunWorker *createWorker(RunControl *runControl)
{
    IosDevice::ConstPtr dev = std::dynamic_pointer_cast<const IosDevice>(runControl->device());
    const bool isIosDeviceType = runControl->device()->type() == Ios::Constants::IOS_DEVICE_TYPE;
    const bool isIosDeviceInstance = bool(dev);
    // type info and device class must match
    QTC_ASSERT(isIosDeviceInstance == isIosDeviceType,
               runControl->postMessage(Tr::tr("Internal error."), ErrorMessageFormat); return nullptr);
    DebuggerRunParameters rp = DebuggerRunParameters::fromRunControl(runControl);
    // TODO cannot use setupPortsGatherer(), because that also requests
    // the "debugChannel", which then results in runControl trying to retrieve ports&URL for that
    // via IDevice, which doesn't really work with the iOS setup, and also completely changes
    // how the debuggerRecipe() works, breaking debugging on iOS <= 16 devices.
    if (rp.isQmlDebugging())
        runControl->requestQmlChannel();

    const IosDeviceTypeAspect::Data *data = runControl->aspectData<IosDeviceTypeAspect>();
    QTC_ASSERT(data, runControl->postMessage("Broken IosDeviceTypeAspect setup.", ErrorMessageFormat);
               return nullptr);
    rp.setDisplayName(data->applicationName);
    rp.setContinueAfterAttach(true);

    RunWorker *runner = nullptr;
    const bool isIosRunner = !isIosDeviceInstance /*== simulator */ || dev->handler() == IosDevice::Handler::IosTool;
    if (isIosRunner) {
        const DebugInfo debugInfo{rp.isQmlDebugging() ? QmlDebuggerServices : NoQmlDebugServices,
                                  rp.isCppDebugging()};
        runner = new IosRunner(runControl, debugInfo);
    } else {
        QTC_ASSERT(rp.isCppDebugging(),
                   // TODO: The message is not shown currently, fix me before 17.0.
                   runControl->postMessage(msgOnlyCppDebuggingSupported(), ErrorMessageFormat);
                   return nullptr);
        if (rp.isQmlDebugging()) {
            rp.setQmlDebugging(false);
            // TODO: The message is not shown currently, fix me before 17.0.
            runControl->postMessage(msgOnlyCppDebuggingSupported(), LogMessageFormat);
        }
        rp.setInferiorExecutable(data->localExecutable);
        runner = new RecipeRunner(runControl, deviceCtlRecipe(runControl, /*startStopped=*/ true));
    }

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

    auto debugger = createDebuggerWorker(runControl, rp, [runControl, isIosRunner](DebuggerRunParameters &rp) {
        if (!isIosRunner)
            return;
        parametersModifier(runControl, rp);
    });
    debugger->addStartDependency(runner);
    return debugger;
}

IosDebugWorkerFactory::IosDebugWorkerFactory()
{
    setProducer([](RunControl *runControl) { return createWorker(runControl); });
    addSupportedRunMode(ProjectExplorer::Constants::DEBUG_RUN_MODE);
    addSupportedRunConfig(Constants::IOS_RUNCONFIG_ID);
}

IosQmlProfilerWorkerFactory::IosQmlProfilerWorkerFactory()
{
    setProducer([](RunControl *runControl) {
        auto runner = new IosRunner(runControl, {QmlProfilerServices});

        auto profiler = runControl->createWorker(ProjectExplorer::Constants::QML_PROFILER_RUNNER);
        profiler->addStartDependency(runner);
        return profiler;
    });
    addSupportedRunMode(ProjectExplorer::Constants::QML_PROFILER_RUN_MODE);
    addSupportedRunConfig(Constants::IOS_RUNCONFIG_ID);
}

} // Ios::Internal
