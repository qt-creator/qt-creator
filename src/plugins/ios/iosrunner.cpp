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

#include <qmldebug/qmloutputparser.h>

#include <utils/fileutils.h>
#include <utils/qtcprocess.h>
#include <utils/stringutils.h>
#include <utils/temporaryfile.h>
#include <utils/url.h>
#include <utils/utilsicons.h>

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

    Target *target = runControl->target();
    const Id devId = RunDeviceKitAspect::deviceId(target->kit());
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
    QSettings settings(bundlePath.pathAppended("Info.plist").toString(), QSettings::NativeFormat);
    return settings.value(QString::fromLatin1("CFBundleIdentifier")).toString();
}

struct AppInfo
{
    QUrl pathOnDevice;
    qint64 processIdentifier = -1;
};

class DeviceCtlRunnerBase : public RunWorker
{
public:
    DeviceCtlRunnerBase(RunControl *runControl);

    void start();
    qint64 processIdentifier() const { return m_processIdentifier; }

protected:
    GroupItem findApp(const QString &bundleIdentifier, Storage<AppInfo> appInfo);
    GroupItem findProcess(Storage<AppInfo> &appInfo);
    void reportStoppedImpl();

    IosDevice::ConstPtr m_device;
    QStringList m_arguments;
    qint64 m_processIdentifier = -1;

private:
    GroupItem killProcess(Storage<AppInfo> &appInfo);
    virtual GroupItem launchTask(const QString &bundleIdentifier) = 0;

    FilePath m_bundlePath;
    std::unique_ptr<TaskTree> m_startTask;
};

class DeviceCtlPollingRunner final : public DeviceCtlRunnerBase
{
public:
    DeviceCtlPollingRunner(RunControl *runControl);

    void stop();

private:
    GroupItem launchTask(const QString &bundleIdentifier);
    void checkProcess();

    std::unique_ptr<TaskTree> m_stopTask;
    std::unique_ptr<TaskTree> m_pollTask;
    QTimer m_pollTimer;
};

class DeviceCtlRunner final : public DeviceCtlRunnerBase
{
public:
    DeviceCtlRunner(RunControl *runControl);

    void stop();

    void setStartStopped(bool startStopped) { m_startStopped = startStopped; }

private:
    GroupItem launchTask(const QString &bundleIdentifier);

    Process m_process;
    std::unique_ptr<TemporaryFile> m_deviceCtlOutput;
    std::unique_ptr<TaskTree> m_processIdTask;
    bool m_startStopped = false;
};

DeviceCtlRunnerBase::DeviceCtlRunnerBase(RunControl *runControl)
    : RunWorker(runControl)
{
    setId("IosDeviceCtlRunner");
    const IosDeviceTypeAspect::Data *data = runControl->aspectData<IosDeviceTypeAspect>();
    QTC_ASSERT(data, return);
    m_bundlePath = data->bundleDirectory;
    m_arguments = ProcessArgs::splitArgs(runControl->commandLine().arguments(), OsTypeMac);
    m_device = std::dynamic_pointer_cast<const IosDevice>(RunDeviceKitAspect::device(runControl->kit()));
}

GroupItem DeviceCtlRunnerBase::findApp(const QString &bundleIdentifier, Storage<AppInfo> appInfo)
{
    const auto onSetup = [this](Process &process) {
        if (!m_device)
            return SetupResult::StopWithSuccess; // don't block the following tasks
        process.setCommand({FilePath::fromString("/usr/bin/xcrun"),
                            {"devicectl",
                             "device",
                             "info",
                             "apps",
                             "--device",
                             m_device->uniqueInternalDeviceId(),
                             "--quiet",
                             "--json-output",
                             "-"}});
        return SetupResult::Continue;
    };
    const auto onDone = [this, bundleIdentifier, appInfo](const Process &process) {
        if (process.error() != QProcess::UnknownError) {
            reportFailure(Tr::tr("Failed to run devicectl: %1.").arg(process.errorString()));
            return DoneResult::Error;
        }
        const expected_str<QUrl> pathOnDevice = parseAppInfo(process.rawStdOut(), bundleIdentifier);
        if (pathOnDevice) {
            appInfo->pathOnDevice = *pathOnDevice;
            return DoneResult::Success;
        }
        reportFailure(pathOnDevice.error());
        return DoneResult::Error;
    };
    return ProcessTask(onSetup, onDone);
}

GroupItem DeviceCtlRunnerBase::findProcess(Storage<AppInfo> &appInfo)
{
    const auto onSetup = [this, appInfo](Process &process) {
        if (!m_device || appInfo->pathOnDevice.isEmpty())
            return SetupResult::StopWithSuccess; // don't block the following tasks
        process.setCommand(
            {FilePath::fromString("/usr/bin/xcrun"),
             {"devicectl",
              "device",
              "info",
              "processes",
              "--device",
              m_device->uniqueInternalDeviceId(),
              "--quiet",
              "--json-output",
              "-",
              "--filter",
              QLatin1String("executable.path BEGINSWITH '%1'").arg(appInfo->pathOnDevice.path())}});
        return SetupResult::Continue;
    };
    const auto onDone = [this, appInfo](const Process &process) {
        const Utils::expected_str<qint64> pid = parseProcessIdentifier(process.rawStdOut());
        if (pid) {
            appInfo->processIdentifier = *pid;
            return DoneResult::Success;
        }
        reportFailure(pid.error());
        return DoneResult::Error;
    };
    return ProcessTask(onSetup, onDone);
}

GroupItem DeviceCtlRunnerBase::killProcess(Storage<AppInfo> &appInfo)
{
    const auto onSetup = [this, appInfo](Process &process) {
        if (!m_device || appInfo->processIdentifier < 0)
            return SetupResult::StopWithSuccess; // don't block the following tasks
        process.setCommand({FilePath::fromString("/usr/bin/xcrun"),
                            {"devicectl",
                             "device",
                             "process",
                             "signal",
                             "--device",
                             m_device->uniqueInternalDeviceId(),
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

GroupItem DeviceCtlPollingRunner::launchTask(const QString &bundleIdentifier)
{
    const auto onSetup = [this, bundleIdentifier](Process &process) {
        if (!m_device) {
            reportFailure(Tr::tr("Running failed. No iOS device found."));
            return SetupResult::StopWithError;
        }
        process.setCommand({FilePath::fromString("/usr/bin/xcrun"),
                            {"devicectl",
                             "device",
                             "process",
                             "launch",
                             "--device",
                             m_device->uniqueInternalDeviceId(),
                             "--quiet",
                             "--json-output",
                             "-",
                             bundleIdentifier,
                             m_arguments}});
        return SetupResult::Continue;
    };
    const auto onDone = [this](const Process &process, DoneWith result) {
        if (result == DoneWith::Cancel) {
            reportFailure(Tr::tr("Running canceled."));
            return DoneResult::Error;
        }
        if (process.error() != QProcess::UnknownError) {
            reportFailure(Tr::tr("Failed to run devicectl: %1.").arg(process.errorString()));
            return DoneResult::Error;
        }
        const Utils::expected_str<qint64> pid = parseLaunchResult(process.rawStdOut());
        if (pid) {
            m_processIdentifier = *pid;
            m_pollTimer.start();
            reportStarted();
            return DoneResult::Success;
        }
        // failure
        reportFailure(pid.error());
        return DoneResult::Error;
    };
    return ProcessTask(onSetup, onDone);
}

void DeviceCtlRunnerBase::reportStoppedImpl()
{
    appendMessage(Tr::tr("\"%1\" exited.").arg(m_bundlePath.toUserOutput()),
                  Utils::NormalMessageFormat);
    reportStopped();
}

void DeviceCtlRunnerBase::start()
{
    const QString bundleIdentifier = getBundleIdentifier(m_bundlePath);
    if (bundleIdentifier.isEmpty()) {
        reportFailure(Tr::tr("Failed to determine bundle identifier."));
        return;
    }

    appendMessage(Tr::tr("Running \"%1\" on %2...")
                      .arg(m_bundlePath.toUserOutput(), device()->displayName()),
                  NormalMessageFormat);

    // If the app is already running, we should first kill it, then launch again.
    // Usually deployment already kills the running app, but we support running without
    // deployment. Restarting is then e.g. needed if the app arguments changed.
    // Unfortunately the information about running processes only includes the path
    // on device and processIdentifier.
    // So we find out if the app is installed, and its path on device.
    // Check if a process is running for that path, and get its processIdentifier.
    // Try to kill that.
    // Then launch the app (again).
    Storage<AppInfo> appInfo;
    m_startTask.reset(new TaskTree(Group{
        sequential,
        appInfo,
        findApp(bundleIdentifier, appInfo),
        findProcess(appInfo),
        killProcess(appInfo),
        launchTask(bundleIdentifier)}));
    m_startTask->start();
}

DeviceCtlPollingRunner::DeviceCtlPollingRunner(RunControl *runControl)
    : DeviceCtlRunnerBase(runControl)
{
    using namespace std::chrono_literals;
    m_pollTimer.setInterval(500ms); // not too often since running devicectl takes time
    connect(&m_pollTimer, &QTimer::timeout, this, &DeviceCtlPollingRunner::checkProcess);
}

void DeviceCtlPollingRunner::stop()
{
    // stop polling, we handle the reportStopped in the done handler
    m_pollTimer.stop();
    if (m_pollTask)
        m_pollTask.release()->deleteLater();
    const auto onSetup = [this](Process &process) {
        if (!m_device) {
            reportStoppedImpl();
            return SetupResult::StopWithError;
        }
        process.setCommand({FilePath::fromString("/usr/bin/xcrun"),
                            {"devicectl",
                             "device",
                             "process",
                             "signal",
                             "--device",
                             m_device->uniqueInternalDeviceId(),
                             "--quiet",
                             "--json-output",
                             "-",
                             "--signal",
                             "SIGKILL",
                             "--pid",
                             QString::number(m_processIdentifier)}});
        return SetupResult::Continue;
    };
    const auto onDone = [this](const Process &process) {
        if (process.error() != QProcess::UnknownError) {
            reportFailure(Tr::tr("Failed to run devicectl: %1.").arg(process.errorString()));
            return DoneResult::Error;
        }
        const Utils::expected_str<QJsonValue> resultValue = parseDevicectlResult(
            process.rawStdOut());
        if (!resultValue) {
            reportFailure(resultValue.error());
            return DoneResult::Error;
        }
        reportStoppedImpl();
        return DoneResult::Success;
    };
    m_stopTask.reset(new TaskTree(Group{ProcessTask(onSetup, onDone)}));
    m_stopTask->start();
}

void DeviceCtlPollingRunner::checkProcess()
{
    if (m_pollTask)
        return;
    const auto onSetup = [this](Process &process) {
        if (!m_device)
            return SetupResult::StopWithError;
        process.setCommand(
            {FilePath::fromString("/usr/bin/xcrun"),
             {"devicectl",
              "device",
              "info",
              "processes",
              "--device",
              m_device->uniqueInternalDeviceId(),
              "--quiet",
              "--json-output",
              "-",
              "--filter",
              QLatin1String("processIdentifier == %1").arg(QString::number(m_processIdentifier))}});
        return SetupResult::Continue;
    };
    const auto onDone = [this](const Process &process) {
        const Utils::expected_str<QJsonValue> resultValue = parseDevicectlResult(
            process.rawStdOut());
        if (!resultValue || (*resultValue)["runningProcesses"].toArray().size() < 1) {
            // no process with processIdentifier found, or some error occurred, device disconnected
            // or such, assume "stopped"
            m_pollTimer.stop();
            reportStoppedImpl();
        }
        m_pollTask.release()->deleteLater();
        return DoneResult::Success;
    };
    m_pollTask.reset(new TaskTree(Group{ProcessTask(onSetup, onDone)}));
    m_pollTask->start();
}

DeviceCtlRunner::DeviceCtlRunner(RunControl *runControl)
    : DeviceCtlRunnerBase(runControl)
{}

void DeviceCtlRunner::stop()
{
    if (m_process.isRunning())
        m_process.stop();
}

GroupItem DeviceCtlRunner::launchTask(const QString &bundleIdentifier)
{
    const auto onSetup = [this, bundleIdentifier] {
        if (!m_device) {
            reportFailure(Tr::tr("Running failed. No iOS device found."));
            return false;
        }
        m_deviceCtlOutput.reset(new TemporaryFile("devicectl"));
        if (!m_deviceCtlOutput->open() || m_deviceCtlOutput->fileName().isEmpty()) {
            reportFailure(Tr::tr("Running failed. Failed to create the temporary output file."));
            return false;
        }
        const QStringList startStoppedArg = m_startStopped ? QStringList("--start-stopped")
                                                           : QStringList();
        const QStringList arguments = QStringList(
                                          {"devicectl",
                                           "device",
                                           "process",
                                           "launch",
                                           "--device",
                                           m_device->uniqueInternalDeviceId(),
                                           "--quiet",
                                           "--json-output",
                                           m_deviceCtlOutput->fileName()})
                                      + startStoppedArg
                                      + QStringList({"--console", bundleIdentifier}) + m_arguments;
        m_process.setCommand({FilePath::fromString("/usr/bin/xcrun"), arguments});
        connect(&m_process, &Process::started, this, [this, bundleIdentifier] {
            // devicectl does report the process ID in its json output, but that is broken
            // for --console. When that is used, the json output is only written after the process
            // finished, which is not helpful.
            // Manually find the process ID for the bundle identifier.
            Storage<AppInfo> appInfo;
            m_processIdTask.reset(new TaskTree(Group{
                sequential,
                appInfo,
                findApp(bundleIdentifier, appInfo),
                findProcess(appInfo),
                onGroupDone([this, appInfo](DoneWith doneWith) {
                    if (doneWith == DoneWith::Success) {
                        m_processIdentifier = appInfo->processIdentifier;
                        reportStarted();
                    } else {
                        reportFailure(Tr::tr("Failed to retrieve process ID."));
                    }
                })}));
            m_processIdTask->start();
        });
        connect(&m_process, &Process::done, this, [this] {
            if (m_process.error() != QProcess::UnknownError)
                reportFailure(Tr::tr("Failed to run devicectl: %1.").arg(m_process.errorString()));
            m_deviceCtlOutput->reset();
            m_processIdTask.reset();
            reportStoppedImpl();
        });
        connect(&m_process, &Process::readyReadStandardError, this, [this] {
            appendMessage(m_process.readAllStandardError(), StdErrFormat, false);
        });
        connect(&m_process, &Process::readyReadStandardOutput, this, [this] {
            appendMessage(m_process.readAllStandardOutput(), StdOutFormat, false);
        });

        m_process.start();
        return true;
    };
    return Sync(onSetup);
}

class IosRunner : public RunWorker
{
public:
    IosRunner(RunControl *runControl);
    ~IosRunner() override;

    void setCppDebugging(bool cppDebug);
    void setQmlDebugging(QmlDebugServicesPreset qmlDebugServices);

    void start() override;
    void stop() final;

    Port qmlServerPort() const;
    Port gdbServerPort() const;
    qint64 pid() const;
    bool isAppRunning() const;

private:
    Utils::FilePath bundlePath() const;
    QString deviceId() const;
    IosToolHandler::RunKind runType() const;
    bool cppDebug() const;
    bool qmlDebug() const;

    void handleGotServerPorts(Ios::IosToolHandler *handler, const FilePath &bundlePath,
                              const QString &deviceId, Port gdbPort, Port qmlPort);
    void handleGotInferiorPid(Ios::IosToolHandler *handler, const FilePath &bundlePath,
                              const QString &deviceId, qint64 pid);
    void handleAppOutput(Ios::IosToolHandler *handler, const QString &output);
    void handleMessage(const QString &msg);
    void handleErrorMsg(Ios::IosToolHandler *handler, const QString &msg);
    void handleToolExited(Ios::IosToolHandler *handler, int code);
    void handleFinished(Ios::IosToolHandler *handler);

    IosToolHandler *m_toolHandler = nullptr;
    FilePath m_bundleDir;
    IDeviceConstPtr m_device;
    IosDeviceType m_deviceType;
    bool m_cppDebug = false;
    QmlDebugServicesPreset m_qmlDebugServices = NoQmlDebugServices;

    bool m_cleanExit = false;
    Port m_qmlServerPort;
    Port m_gdbServerPort;
    qint64 m_pid = 0;
};

IosRunner::IosRunner(RunControl *runControl)
    : RunWorker(runControl)
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

void IosRunner::setCppDebugging(bool cppDebug)
{
    m_cppDebug = cppDebug;
}

void IosRunner::setQmlDebugging(QmlDebugServicesPreset qmlDebugServices)
{
    m_qmlDebugServices = qmlDebugServices;
}

FilePath IosRunner::bundlePath() const
{
    return m_bundleDir;
}

QString IosRunner::deviceId() const
{
    IosDevice::ConstPtr dev = std::dynamic_pointer_cast<const IosDevice>(m_device);
    if (!dev)
        return {};
    return dev->uniqueDeviceID();
}

IosToolHandler::RunKind IosRunner::runType() const
{
    if (m_cppDebug)
        return IosToolHandler::DebugRun;
    return IosToolHandler::NormalRun;
}

bool IosRunner::cppDebug() const
{
    return m_cppDebug;
}

bool IosRunner::qmlDebug() const
{
    return m_qmlDebugServices != NoQmlDebugServices;
}

void IosRunner::start()
{
    if (m_toolHandler && isAppRunning())
        m_toolHandler->stop();

    m_cleanExit = false;
    m_qmlServerPort = Port();
    if (!m_bundleDir.exists()) {
        TaskHub::addTask(DeploymentTask(Task::Warning,
            Tr::tr("Could not find %1.").arg(m_bundleDir.toUserOutput())));
        reportFailure();
        return;
    }
    if (m_device->type() == Ios::Constants::IOS_DEVICE_TYPE) {
        IosDevice::ConstPtr iosDevice = std::dynamic_pointer_cast<const IosDevice>(m_device);
        if (!m_device) {
            reportFailure();
            return;
        }
        if (m_qmlDebugServices != NoQmlDebugServices)
            m_qmlServerPort = iosDevice->nextPort();
    } else {
        IosSimulator::ConstPtr sim = std::dynamic_pointer_cast<const IosSimulator>(m_device);
        if (!sim) {
            reportFailure();
            return;
        }
        if (m_qmlDebugServices != NoQmlDebugServices)
            m_qmlServerPort = sim->nextPort();
    }

    m_toolHandler = new IosToolHandler(m_deviceType, this);
    connect(m_toolHandler, &IosToolHandler::appOutput,
            this, &IosRunner::handleAppOutput);
    connect(m_toolHandler, &IosToolHandler::message, this, &IosRunner::handleMessage);
    connect(m_toolHandler, &IosToolHandler::errorMsg, this, &IosRunner::handleErrorMsg);
    connect(m_toolHandler, &IosToolHandler::gotServerPorts,
            this, &IosRunner::handleGotServerPorts);
    connect(m_toolHandler, &IosToolHandler::gotInferiorPid,
            this, &IosRunner::handleGotInferiorPid);
    connect(m_toolHandler, &IosToolHandler::toolExited,
            this, &IosRunner::handleToolExited);
    connect(m_toolHandler, &IosToolHandler::finished,
            this, &IosRunner::handleFinished);

    const CommandLine command = runControl()->commandLine();
    QStringList args = ProcessArgs::splitArgs(command.arguments(), OsTypeMac);
    if (m_qmlServerPort.isValid()) {
        QUrl qmlServer;
        qmlServer.setPort(m_qmlServerPort.number());
        args.append(qmlDebugTcpArguments(m_qmlDebugServices, qmlServer));
    }

    appendMessage(Tr::tr("Starting remote process."), NormalMessageFormat);
    m_toolHandler->requestRunApp(bundlePath(), args, runType(), deviceId());
}

void IosRunner::stop()
{
    if (isAppRunning())
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

    m_gdbServerPort = gdbPort;
    m_qmlServerPort = qmlPort;

    bool prerequisiteOk = false;
    if (cppDebug() && qmlDebug())
        prerequisiteOk = m_gdbServerPort.isValid() && m_qmlServerPort.isValid();
    else if (cppDebug())
        prerequisiteOk = m_gdbServerPort.isValid();
    else if (qmlDebug())
        prerequisiteOk = m_qmlServerPort.isValid();
    else
        prerequisiteOk = true; // Not debugging. Ports not required.


    if (prerequisiteOk)
        reportStarted();
    else
        reportFailure(Tr::tr("Could not get necessary ports for the debugger connection."));
}

void IosRunner::handleGotInferiorPid(IosToolHandler *handler, const FilePath &bundlePath,
                                     const QString &deviceId, qint64 pid)
{
    // Called when debugging on Simulator.
    Q_UNUSED(bundlePath)
    Q_UNUSED(deviceId)

    if (m_toolHandler != handler)
        return;

    m_pid = pid;
    bool prerequisiteOk = false;
    if (m_pid > 0) {
        prerequisiteOk = true;
    } else {
        reportFailure(Tr::tr("Could not get inferior PID."));
        return;
    }

    if (qmlDebug())
        prerequisiteOk = m_qmlServerPort.isValid();

    if (prerequisiteOk)
        reportStarted();
    else
        reportFailure(Tr::tr("Could not get necessary ports for the debugger connection."));
}

void IosRunner::handleAppOutput(IosToolHandler *handler, const QString &output)
{
    Q_UNUSED(handler)
    static const QRegularExpression qmlPortRe(QString::fromLatin1(QML_DEBUGGER_WAITING));
    const QRegularExpressionMatch match = qmlPortRe.match(output);
    QString res(output);
    if (match.hasMatch() && m_qmlServerPort.isValid())
       res.replace(match.captured(1), QString::number(m_qmlServerPort.number()));
    appendMessage(output, StdOutFormat);
}

void IosRunner::handleMessage(const QString &msg)
{
    QString res(msg);
    static const QRegularExpression qmlPortRe(QString::fromLatin1(QML_DEBUGGER_WAITING));
    const QRegularExpressionMatch match = qmlPortRe.match(msg);
    if (match.hasMatch() && m_qmlServerPort.isValid())
        res.replace(match.captured(1), QString::number(m_qmlServerPort.number()));
    appendMessage(res, StdOutFormat);
}

void IosRunner::handleErrorMsg(IosToolHandler *handler, const QString &msg)
{
    Q_UNUSED(handler)
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

void IosRunner::handleToolExited(IosToolHandler *handler, int code)
{
    Q_UNUSED(handler)
    m_cleanExit = (code == 0);
}

void IosRunner::handleFinished(IosToolHandler *handler)
{
    if (m_toolHandler == handler) {
        if (m_cleanExit)
            appendMessage(Tr::tr("Run ended."), NormalMessageFormat);
        else
            appendMessage(Tr::tr("Run ended with error."), ErrorMessageFormat);
        m_toolHandler = nullptr;
    }
    handler->deleteLater();
    reportStopped();
}

qint64 IosRunner::pid() const
{
    return m_pid;
}

bool IosRunner::isAppRunning() const
{
    return m_toolHandler && m_toolHandler->isRunning();
}

Port IosRunner::gdbServerPort() const
{
    return m_gdbServerPort;
}

Port IosRunner::qmlServerPort() const
{
    return m_qmlServerPort;
}

//
// IosQmlProfilerSupport
//

class IosQmlProfilerSupport : public RunWorker
{

public:
    IosQmlProfilerSupport(RunControl *runControl);

private:
    void start() override;
    IosRunner *m_runner = nullptr;
    RunWorker *m_profiler = nullptr;
};

IosQmlProfilerSupport::IosQmlProfilerSupport(RunControl *runControl)
    : RunWorker(runControl)
{
    setId("IosQmlProfilerSupport");

    m_runner = new IosRunner(runControl);
    m_runner->setQmlDebugging(QmlProfilerServices);
    addStartDependency(m_runner);

    m_profiler = runControl->createWorker(ProjectExplorer::Constants::QML_PROFILER_RUNNER);
    m_profiler->addStartDependency(this);
}

void IosQmlProfilerSupport::start()
{
    QTcpServer server;
    const bool isListening = server.listen(QHostAddress::LocalHost)
                          || server.listen(QHostAddress::LocalHostIPv6);
    QTC_ASSERT(isListening, return);

    const Port qmlPort = m_runner->qmlServerPort();
    if (qmlPort.isValid())
        reportStarted();
    else
        reportFailure(Tr::tr("Could not get necessary ports for the profiler connection."));
}

//
// IosDebugSupport
//

class IosDebugSupport : public DebuggerRunTool
{
public:
    IosDebugSupport(RunControl *runControl);

private:
    void start() override;

    IosRunner *m_iosRunner = nullptr;
    DeviceCtlRunner *m_deviceCtlRunner = nullptr;
};

static expected_str<FilePath> findDeviceSdk(IosDevice::ConstPtr dev)
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
        return Utils::make_unexpected(
            Tr::tr("Could not find device specific debug symbols at %1. "
                   "Debugging initialization will be slow until you open the Organizer window of "
                   "Xcode with the device connected to have the symbols generated.")
                .arg(symbolsPathCandidates.constFirst().toUserOutput()));
    }
    return deviceSdk;
}

IosDebugSupport::IosDebugSupport(RunControl *runControl)
    : DebuggerRunTool(runControl)
{
    setId("IosDebugSupport");

    IosDevice::ConstPtr dev = std::dynamic_pointer_cast<const IosDevice>(device());

    if (dev->type() == Ios::Constants::IOS_SIMULATOR_TYPE
        || dev->handler() == IosDevice::Handler::IosTool) {
        m_iosRunner = new IosRunner(runControl);
        m_iosRunner->setCppDebugging(isCppDebugging());
        m_iosRunner->setQmlDebugging(isQmlDebugging() ? QmlDebuggerServices : NoQmlDebugServices);
        addStartDependency(m_iosRunner);
    } else {
        QTC_CHECK(isCppDebugging());
        m_deviceCtlRunner = new DeviceCtlRunner(runControl);
        m_deviceCtlRunner->setStartStopped(true);
        addStartDependency(m_deviceCtlRunner);
    }

    DebuggerRunParameters &rp = runParameters();
    if (device()->type() == Ios::Constants::IOS_DEVICE_TYPE) {
        if (dev->handler() == IosDevice::Handler::DeviceCtl) {
            QTC_CHECK(IosDeviceManager::isDeviceCtlDebugSupported());
            rp.setStartMode(AttachToIosDevice);
            setDeviceUuid(dev->uniqueInternalDeviceId());
        } else {
            rp.setStartMode(AttachToRemoteProcess);
        }
        setIosPlatform("remote-ios");
        const expected_str<FilePath> deviceSdk = findDeviceSdk(dev);

        if (!deviceSdk)
            TaskHub::addTask(DeploymentTask(Task::Warning, deviceSdk.error()));
        else
            setDeviceSymbolsRoot(deviceSdk->path());
    } else {
        rp.setStartMode(AttachToLocalProcess);
        setIosPlatform("ios-simulator");
    }
}

void IosDebugSupport::start()
{
    const IosDeviceTypeAspect::Data *data = runControl()->aspectData<IosDeviceTypeAspect>();
    QTC_ASSERT(data, reportFailure("Broken IosDeviceTypeAspect setup."); return);
    setRunControlName(data->applicationName);
    setContinueAfterAttach(true);

    IosDevice::ConstPtr dev = std::dynamic_pointer_cast<const IosDevice>(device());
    if (dev->type() == Ios::Constants::IOS_DEVICE_TYPE
        && dev->handler() == IosDevice::Handler::DeviceCtl) {
        const auto msgOnlyCppDebuggingSupported = [] {
            return Tr::tr("Only C++ debugging is supported for devices with iOS 17 and later.");
        };
        if (!isCppDebugging()) {
            reportFailure(msgOnlyCppDebuggingSupported());
            return;
        }
        if (isQmlDebugging()) {
            runParameters().isQmlDebugging = false;
            appendMessage(msgOnlyCppDebuggingSupported(), OutputFormat::LogMessageFormat, true);
        }
        setAttachPid(m_deviceCtlRunner->processIdentifier());
        setInferiorExecutable(data->localExecutable);
        DebuggerRunTool::start();
        return;
    }

    if (!m_iosRunner->isAppRunning()) {
        reportFailure(Tr::tr("Application not running."));
        return;
    }

    Port gdbServerPort = m_iosRunner->gdbServerPort();
    Port qmlServerPort = m_iosRunner->qmlServerPort();
    setAttachPid(ProcessHandle(m_iosRunner->pid()));

    const bool cppDebug = isCppDebugging();
    const bool qmlDebug = isQmlDebugging();
    if (cppDebug) {
        setInferiorExecutable(data->localExecutable);
        setRemoteChannel("connect://localhost:" + gdbServerPort.toString());

        QString bundlePath = data->bundleDirectory.toString();
        bundlePath.chop(4);
        FilePath dsymPath = FilePath::fromString(bundlePath.append(".dSYM"));
        if (dsymPath.exists()
                && dsymPath.lastModified() < data->localExecutable.lastModified()) {
            TaskHub::addTask(DeploymentTask(Task::Warning,
                Tr::tr("The dSYM %1 seems to be outdated, it might confuse the debugger.")
                    .arg(dsymPath.toUserOutput())));
        }
    }

    QUrl qmlServer;
    if (qmlDebug) {
        QTcpServer server;
        const bool isListening = server.listen(QHostAddress::LocalHost)
                              || server.listen(QHostAddress::LocalHostIPv6);
        QTC_ASSERT(isListening, return);
        qmlServer.setHost(server.serverAddress().toString());
        if (!cppDebug)
            runParameters().setStartMode(AttachToRemoteServer);
    }

    if (qmlServerPort.isValid())
        qmlServer.setPort(qmlServerPort.number());

    setQmlServer(qmlServer);

    DebuggerRunTool::start();
}

// Factories

IosRunWorkerFactory::IosRunWorkerFactory()
{
    setProducer([](RunControl *control) -> RunWorker * {
        IosDevice::ConstPtr iosdevice = std::dynamic_pointer_cast<const IosDevice>(control->device());
        if (iosdevice && iosdevice->handler() == IosDevice::Handler::DeviceCtl) {
            if (IosDeviceManager::isDeviceCtlOutputSupported())
                return new DeviceCtlRunner(control);
            // TODO Remove the polling runner when we decide not to support iOS 17+ devices
            // with Xcode < 15.4 at all
            return new DeviceCtlPollingRunner(control);
        }
        control->setIcon(Icons::RUN_SMALL_TOOLBAR);
        control->setDisplayName(QString("Run on %1")
                                       .arg(iosdevice ? iosdevice->displayName() : QString()));
        return new IosRunner(control);
    });
    addSupportedRunMode(ProjectExplorer::Constants::NORMAL_RUN_MODE);
    addSupportedRunConfig(Constants::IOS_RUNCONFIG_ID);
}

IosDebugWorkerFactory::IosDebugWorkerFactory()
{
    setProduct<IosDebugSupport>();
    addSupportedRunMode(ProjectExplorer::Constants::DEBUG_RUN_MODE);
    addSupportedRunConfig(Constants::IOS_RUNCONFIG_ID);
}

IosQmlProfilerWorkerFactory::IosQmlProfilerWorkerFactory()
{
    setProduct<IosQmlProfilerSupport>();
    addSupportedRunMode(ProjectExplorer::Constants::QML_PROFILER_RUN_MODE);
    addSupportedRunConfig(Constants::IOS_RUNCONFIG_ID);
}

} // Ios::Internal
