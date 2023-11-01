// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "iosrunner.h"

#include "iosconfigurations.h"
#include "iosconstants.h"
#include "iosdevice.h"
#include "iosrunconfiguration.h"
#include "iossimulator.h"
#include "iostoolhandler.h"
#include "iostr.h"

#include <debugger/debuggerconstants.h>
#include <debugger/debuggerkitaspect.h>
#include <debugger/debuggerplugin.h>
#include <debugger/debuggerruncontrol.h>

#include <projectexplorer/kitaspects.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/runconfigurationaspects.h>
#include <projectexplorer/target.h>
#include <projectexplorer/taskhub.h>
#include <projectexplorer/toolchain.h>

#include <qmldebug/qmldebugcommandlinearguments.h>
#include <qmldebug/qmloutputparser.h>

#include <utils/fileutils.h>
#include <utils/process.h>
#include <utils/url.h>
#include <utils/utilsicons.h>

#include <QDateTime>
#include <QDir>
#include <QMessageBox>
#include <QRegularExpression>
#include <QSettings>
#include <QTcpServer>
#include <QTime>

#include <stdio.h>
#include <fcntl.h>
#ifdef Q_OS_UNIX
#include <unistd.h>
#else
#include <io.h>
#endif
#include <signal.h>

using namespace Debugger;
using namespace ProjectExplorer;
using namespace Utils;

namespace Ios::Internal {

static void stopRunningRunControl(RunControl *runControl)
{
    static QMap<Id, QPointer<RunControl>> activeRunControls;

    Target *target = runControl->target();
    Id devId = DeviceKitAspect::deviceId(target->kit());

    // The device can only run an application at a time, if an app is running stop it.
    if (activeRunControls.contains(devId)) {
        if (QPointer<RunControl> activeRunControl = activeRunControls[devId])
            activeRunControl->initiateStop();
        activeRunControls.remove(devId);
    }

    if (devId.isValid())
        activeRunControls[devId] = runControl;
}

class IosRunner : public RunWorker
{
public:
    IosRunner(RunControl *runControl);
    ~IosRunner() override;

    void setCppDebugging(bool cppDebug);
    void setQmlDebugging(QmlDebug::QmlDebugServicesPreset qmlDebugServices);

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
    void handleErrorMsg(Ios::IosToolHandler *handler, const QString &msg);
    void handleToolExited(Ios::IosToolHandler *handler, int code);
    void handleFinished(Ios::IosToolHandler *handler);

    IosToolHandler *m_toolHandler = nullptr;
    FilePath m_bundleDir;
    IDeviceConstPtr m_device;
    IosDeviceType m_deviceType;
    bool m_cppDebug = false;
    QmlDebug::QmlDebugServicesPreset m_qmlDebugServices = QmlDebug::NoQmlDebugServices;

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
    const IosDeviceTypeAspect::Data *data = runControl->aspect<IosDeviceTypeAspect>();
    QTC_ASSERT(data, return);
    m_bundleDir = data->bundleDirectory;
    m_device = DeviceKitAspect::device(runControl->kit());
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

void IosRunner::setQmlDebugging(QmlDebug::QmlDebugServicesPreset qmlDebugServices)
{
    m_qmlDebugServices = qmlDebugServices;
}

FilePath IosRunner::bundlePath() const
{
    return m_bundleDir;
}

QString IosRunner::deviceId() const
{
    IosDevice::ConstPtr dev = m_device.dynamicCast<const IosDevice>();
    if (!dev)
        return QString();
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
    return m_qmlDebugServices != QmlDebug::NoQmlDebugServices;
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
        IosDevice::ConstPtr iosDevice = m_device.dynamicCast<const IosDevice>();
        if (m_device.isNull()) {
            reportFailure();
            return;
        }
        if (m_qmlDebugServices != QmlDebug::NoQmlDebugServices)
            m_qmlServerPort = iosDevice->nextPort();
    } else {
        IosSimulator::ConstPtr sim = m_device.dynamicCast<const IosSimulator>();
        if (sim.isNull()) {
            reportFailure();
            return;
        }
        if (m_qmlDebugServices != QmlDebug::NoQmlDebugServices)
            m_qmlServerPort = sim->nextPort();
    }

    m_toolHandler = new IosToolHandler(m_deviceType, this);
    connect(m_toolHandler, &IosToolHandler::appOutput,
            this, &IosRunner::handleAppOutput);
    connect(m_toolHandler, &IosToolHandler::errorMsg,
            this, &IosRunner::handleErrorMsg);
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
        args.append(QmlDebug::qmlDebugTcpArguments(m_qmlDebugServices, qmlServer));
    }

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
    QRegularExpression qmlPortRe("QML Debugger: Waiting for connection on port ([0-9]+)...");
    const QRegularExpressionMatch match = qmlPortRe.match(output);
    QString res(output);
    if (match.hasMatch() && m_qmlServerPort.isValid())
       res.replace(match.captured(1), QString::number(m_qmlServerPort.number()));
    appendMessage(output, StdOutFormat);
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
    QRegularExpression qmlPortRe("QML Debugger: Waiting for connection on port ([0-9]+)...");
    const QRegularExpressionMatch match = qmlPortRe.match(msg);
    if (match.hasMatch() && m_qmlServerPort.isValid())
       res.replace(match.captured(1), QString::number(m_qmlServerPort.number()));

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
// IosRunner
//

class IosRunSupport : public IosRunner
{
public:
    explicit IosRunSupport(RunControl *runControl);
    ~IosRunSupport() override;

private:
    void start() override;
};

IosRunSupport::IosRunSupport(RunControl *runControl)
    : IosRunner(runControl)
{
    setId("IosRunSupport");
    runControl->setIcon(Icons::RUN_SMALL_TOOLBAR);
    runControl->setDisplayName(QString("Run on %1")
                                   .arg(device().isNull() ? QString() : device()->displayName()));
}

IosRunSupport::~IosRunSupport()
{
    stop();
}

void IosRunSupport::start()
{
    appendMessage(Tr::tr("Starting remote process."), NormalMessageFormat);
    IosRunner::start();
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
    m_runner->setQmlDebugging(QmlDebug::QmlProfilerServices);
    addStartDependency(m_runner);

    m_profiler = runControl->createWorker(ProjectExplorer::Constants::QML_PROFILER_RUNNER);
    m_profiler->addStartDependency(this);
}

void IosQmlProfilerSupport::start()
{
    QUrl serverUrl;
    QTcpServer server;
    const bool isListening = server.listen(QHostAddress::LocalHost)
                          || server.listen(QHostAddress::LocalHostIPv6);
    QTC_ASSERT(isListening, return);
    serverUrl.setScheme(Utils::urlTcpScheme());
    serverUrl.setHost(server.serverAddress().toString());

    Port qmlPort = m_runner->qmlServerPort();
    serverUrl.setPort(qmlPort.number());
    m_profiler->recordData("QmlServerUrl", serverUrl);
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

    const QString m_dumperLib;
    IosRunner *m_runner;
};

IosDebugSupport::IosDebugSupport(RunControl *runControl)
    : DebuggerRunTool(runControl)
{
    setId("IosDebugSupport");

    m_runner = new IosRunner(runControl);
    m_runner->setCppDebugging(isCppDebugging());
    m_runner->setQmlDebugging(isQmlDebugging() ? QmlDebug::QmlDebuggerServices : QmlDebug::NoQmlDebugServices);

    addStartDependency(m_runner);
}

void IosDebugSupport::start()
{
    if (!m_runner->isAppRunning()) {
        reportFailure(Tr::tr("Application not running."));
        return;
    }

    if (device()->type() == Ios::Constants::IOS_DEVICE_TYPE) {
        IosDevice::ConstPtr dev = device().dynamicCast<const IosDevice>();
        setStartMode(AttachToRemoteProcess);
        setIosPlatform("remote-ios");
        const QString osVersion = dev->osVersion();
        const QString cpuArchitecture = dev->cpuArchitecture();
        const FilePaths symbolsPathCandidates = {
            FilePath::fromString(QDir::homePath() + "/Library/Developer/Xcode/iOS DeviceSupport/"
                                 + osVersion + " " + cpuArchitecture + "/Symbols"),
            FilePath::fromString(QDir::homePath() + "/Library/Developer/Xcode/iOS DeviceSupport/"
                                 + osVersion + "/Symbols"),
            IosConfigurations::developerPath().pathAppended(
                "Platforms/iPhoneOS.platform/DeviceSupport/" + osVersion + "/Symbols")};
        const FilePath deviceSdk = Utils::findOrDefault(symbolsPathCandidates, &FilePath::isDir);

        if (deviceSdk.isEmpty()) {
            TaskHub::addTask(DeploymentTask(
                Task::Warning,
                Tr::tr("Could not find device specific debug symbols at %1. "
                       "Debugging initialization will be slow until you open the Organizer window of "
                       "Xcode with the device connected to have the symbols generated.")
                    .arg(symbolsPathCandidates.constFirst().toUserOutput())));
        }
        setDeviceSymbolsRoot(deviceSdk.toString());
    } else {
        setStartMode(AttachToLocalProcess);
        setIosPlatform("ios-simulator");
    }

    const IosDeviceTypeAspect::Data *data = runControl()->aspect<IosDeviceTypeAspect>();
    QTC_ASSERT(data, reportFailure("Broken IosDeviceTypeAspect setup."); return);

    setRunControlName(data->applicationName);
    setContinueAfterAttach(true);

    Port gdbServerPort = m_runner->gdbServerPort();
    Port qmlServerPort = m_runner->qmlServerPort();
    setAttachPid(ProcessHandle(m_runner->pid()));

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
            setStartMode(AttachToRemoteServer);
    }

    if (qmlServerPort.isValid())
        qmlServer.setPort(qmlServerPort.number());

    setQmlServer(qmlServer);

    DebuggerRunTool::start();
}

// Factories

IosRunWorkerFactory::IosRunWorkerFactory()
{
    setProduct<IosRunSupport>();
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
