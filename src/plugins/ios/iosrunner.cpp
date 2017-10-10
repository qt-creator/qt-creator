/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "iosbuildstep.h"
#include "iosconfigurations.h"
#include "iosdevice.h"
#include "iosmanager.h"
#include "iosrunconfiguration.h"
#include "iosrunner.h"
#include "iossimulator.h"
#include "iosconstants.h"

#include <debugger/debuggerplugin.h>
#include <debugger/debuggerkitinformation.h>
#include <debugger/debuggerruncontrol.h>

#include <projectexplorer/kitinformation.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <projectexplorer/taskhub.h>
#include <projectexplorer/toolchain.h>

#include <qmldebug/qmldebugcommandlinearguments.h>

#include <qtsupport/qtkitinformation.h>

#include <utils/fileutils.h>
#include <utils/qtcprocess.h>
#include <utils/utilsicons.h>

#include <QDateTime>
#include <QDir>
#include <QMessageBox>
#include <QRegExp>
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

namespace Ios {
namespace Internal {

static void stopRunningRunControl(RunControl *runControl)
{
    static QMap<Core::Id, QPointer<RunControl>> activeRunControls;

    RunConfiguration *runConfig = runControl->runConfiguration();
    Target *target = runConfig->target();
    Core::Id devId = DeviceKitInformation::deviceId(target->kit());

    // The device can only run an application at a time, if an app is running stop it.
    if (activeRunControls.contains(devId)) {
        if (QPointer<RunControl> activeRunControl = activeRunControls[devId])
            activeRunControl->initiateStop();
        activeRunControls.remove(devId);
    }

    if (devId.isValid())
        activeRunControls[devId] = runControl;
}

IosRunner::IosRunner(RunControl *runControl)
    : RunWorker(runControl)
{
    stopRunningRunControl(runControl);
    auto runConfig = qobject_cast<IosRunConfiguration *>(runControl->runConfiguration());
    m_bundleDir = runConfig->bundleDirectory().toString();
    m_arguments = QStringList(runConfig->commandLineArguments());
    m_device = DeviceKitInformation::device(runConfig->target()->kit());
    m_deviceType = runConfig->deviceType();
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

QString IosRunner::bundlePath()
{
    return m_bundleDir;
}

QStringList IosRunner::extraArgs()
{
    QStringList res = m_arguments;
    if (m_qmlServerPort.isValid())
        res << QmlDebug::qmlDebugTcpArguments(m_qmlDebugServices, m_qmlServerPort);
    return res;
}

QString IosRunner::deviceId()
{
    IosDevice::ConstPtr dev = m_device.dynamicCast<const IosDevice>();
    if (!dev)
        return QString();
    return dev->uniqueDeviceID();
}

IosToolHandler::RunKind IosRunner::runType()
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

QmlDebug::QmlDebugServicesPreset IosRunner::qmlDebugServices() const
{
    return m_qmlDebugServices;
}

void IosRunner::start()
{
    if (m_toolHandler && isAppRunning())
        m_toolHandler->stop();

    m_cleanExit = false;
    m_qmlServerPort = Port();
    if (!QFileInfo::exists(m_bundleDir)) {
        TaskHub::addTask(Task::Warning,
                         tr("Could not find %1.").arg(m_bundleDir),
                         ProjectExplorer::Constants::TASK_CATEGORY_DEPLOYMENT);
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
    m_toolHandler->requestRunApp(bundlePath(), extraArgs(), runType(), deviceId());
}

void IosRunner::stop()
{
    if (isAppRunning())
        m_toolHandler->stop();
}

void IosRunner::handleGotServerPorts(IosToolHandler *handler, const QString &bundlePath,
                                     const QString &deviceId, Port gdbPort,
                                     Port qmlPort)
{
    // Called when debugging on Device.
    Q_UNUSED(bundlePath); Q_UNUSED(deviceId);

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
        reportFailure(tr("Could not get necessary ports for the debugger connection."));
}

void IosRunner::handleGotInferiorPid(IosToolHandler *handler, const QString &bundlePath,
                                     const QString &deviceId, qint64 pid)
{
    // Called when debugging on Simulator.
    Q_UNUSED(bundlePath); Q_UNUSED(deviceId);

    if (m_toolHandler != handler)
        return;

    m_pid = pid;
    bool prerequisiteOk = false;
    if (m_pid > 0) {
        prerequisiteOk = true;
    } else {
        reportFailure(tr("Could not get inferior PID."));
        return;
    }

    if (qmlDebug())
        prerequisiteOk = m_qmlServerPort.isValid();

    if (prerequisiteOk)
        reportStarted();
    else
        reportFailure(tr("Could not get necessary ports the debugger connection."));
}

void IosRunner::handleAppOutput(IosToolHandler *handler, const QString &output)
{
    Q_UNUSED(handler);
    QRegExp qmlPortRe("QML Debugger: Waiting for connection on port ([0-9]+)...");
    int index = qmlPortRe.indexIn(output);
    QString res(output);
    if (index != -1 && m_qmlServerPort.isValid())
       res.replace(qmlPortRe.cap(1), QString::number(m_qmlServerPort.number()));
    appendMessage(output, StdOutFormat);
    appOutput(res);
}

void IosRunner::handleErrorMsg(IosToolHandler *handler, const QString &msg)
{
    Q_UNUSED(handler);
    QString res(msg);
    QString lockedErr ="Unexpected reply: ELocked (454c6f636b6564) vs OK (4f4b)";
    if (msg.contains("AMDeviceStartService returned -402653150")) {
        TaskHub::addTask(Task::Warning,
                         tr("Run failed. The settings in the Organizer window of Xcode might be incorrect."),
                         ProjectExplorer::Constants::TASK_CATEGORY_DEPLOYMENT);
    } else if (res.contains(lockedErr)) {
        QString message = tr("The device is locked, please unlock.");
        TaskHub::addTask(Task::Error, message,
                         ProjectExplorer::Constants::TASK_CATEGORY_DEPLOYMENT);
        res.replace(lockedErr, message);
    }
    QRegExp qmlPortRe("QML Debugger: Waiting for connection on port ([0-9]+)...");
    int index = qmlPortRe.indexIn(msg);
    if (index != -1 && m_qmlServerPort.isValid())
       res.replace(qmlPortRe.cap(1), QString::number(m_qmlServerPort.number()));

    appendMessage(res, StdErrFormat);
    errorMsg(res);
}

void IosRunner::handleToolExited(IosToolHandler *handler, int code)
{
    Q_UNUSED(handler);
    m_cleanExit = (code == 0);
}

void IosRunner::handleFinished(IosToolHandler *handler)
{
    if (m_toolHandler == handler) {
        if (m_cleanExit)
            appendMessage(tr("Run ended."), NormalMessageFormat);
        else
            appendMessage(tr("Run ended with error."), ErrorMessageFormat);
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

Utils::Port IosRunner::gdbServerPort() const
{
    return m_gdbServerPort;
}

Utils::Port IosRunner::qmlServerPort() const
{
    return m_qmlServerPort;
}

//
// IosRunner
//

IosRunSupport::IosRunSupport(RunControl *runControl)
    : IosRunner(runControl)
{
    runControl->setIcon(Icons::RUN_SMALL_TOOLBAR);
    QString displayName = QString("Run on %1").arg(device().isNull() ? QString() : device()->displayName());
    runControl->setDisplayName(displayName);
}

IosRunSupport::~IosRunSupport()
{
    stop();
}

void IosRunSupport::start()
{
    appendMessage(tr("Starting remote process."), NormalMessageFormat);
    IosRunner::start();
}

void IosRunSupport::stop()
{
    IosRunner::stop();
}

//
// IosQmlProfilerSupport
//

IosQmlProfilerSupport::IosQmlProfilerSupport(RunControl *runControl)
    : RunWorker(runControl)
{
    setDisplayName("IosAnalyzeSupport");

    auto iosRunConfig = qobject_cast<IosRunConfiguration *>(runControl->runConfiguration());
    StandardRunnable runnable;
    runnable.executable = iosRunConfig->localExecutable().toUserOutput();
    runnable.commandLineArguments = iosRunConfig->commandLineArguments();
    runControl->setDisplayName(iosRunConfig->applicationName());
    runControl->setRunnable(runnable);

    m_runner = new IosRunner(runControl);
    m_runner->setQmlDebugging(QmlDebug::QmlProfilerServices);
    addStartDependency(m_runner);

    m_profiler = runControl->createWorker(runControl->runMode());
    m_profiler->addStartDependency(this);
}

void IosQmlProfilerSupport::start()
{
    QUrl serverUrl;
    QTcpServer server;
    QTC_ASSERT(server.listen(QHostAddress::LocalHost)
               || server.listen(QHostAddress::LocalHostIPv6), return);
    serverUrl.setHost(server.serverAddress().toString());

    Port qmlPort = m_runner->qmlServerPort();
    serverUrl.setPort(qmlPort.number());
    m_profiler->recordData("QmlServerUrl", serverUrl);
    if (qmlPort.isValid())
        reportStarted();
    else
        reportFailure(tr("Could not get necessary ports for the profiler connection."));
}

//
// IosDebugSupport
//

IosDebugSupport::IosDebugSupport(RunControl *runControl)
    : Debugger::DebuggerRunTool(runControl)
{
    m_runner = new IosRunner(runControl);
    m_runner->setCppDebugging(isCppDebugging());
    m_runner->setQmlDebugging(isQmlDebugging() ? QmlDebug::QmlDebuggerServices : QmlDebug::NoQmlDebugServices);

    addStartDependency(m_runner);
}

void IosDebugSupport::start()
{
    if (!m_runner->isAppRunning()) {
        reportFailure(tr("Application not running."));
        return;
    }

    RunConfiguration *runConfig = runControl()->runConfiguration();

    if (device()->type() == Ios::Constants::IOS_DEVICE_TYPE) {
        IosDevice::ConstPtr dev = device().dynamicCast<const IosDevice>();
        setStartMode(AttachToRemoteProcess);
        setIosPlatform("remote-ios");
        QString osVersion = dev->osVersion();
        FileName deviceSdk1 = FileName::fromString(QDir::homePath()
                                             + "/Library/Developer/Xcode/iOS DeviceSupport/"
                                             + osVersion + "/Symbols");
        QString deviceSdk;
        if (deviceSdk1.toFileInfo().isDir()) {
            deviceSdk = deviceSdk1.toString();
        } else {
            FileName deviceSdk2 = IosConfigurations::developerPath()
                    .appendPath("Platforms/iPhoneOS.platform/DeviceSupport/")
                    .appendPath(osVersion).appendPath("Symbols");
            if (deviceSdk2.toFileInfo().isDir()) {
                deviceSdk = deviceSdk2.toString();
            } else {
                TaskHub::addTask(Task::Warning, tr(
                  "Could not find device specific debug symbols at %1. "
                  "Debugging initialization will be slow until you open the Organizer window of "
                  "Xcode with the device connected to have the symbols generated.")
                                 .arg(deviceSdk1.toUserOutput()),
                                 ProjectExplorer::Constants::TASK_CATEGORY_DEPLOYMENT);
            }
        }
        setDeviceSymbolsRoot(deviceSdk);
    } else {
        setStartMode(AttachExternal);
        setIosPlatform("ios-simulator");
    }

    auto iosRunConfig = qobject_cast<IosRunConfiguration *>(runConfig);
    setRunControlName(iosRunConfig->applicationName());
    setContinueAfterAttach(true);

    Utils::Port gdbServerPort = m_runner->gdbServerPort();
    Utils::Port qmlServerPort = m_runner->qmlServerPort();
    setAttachPid(ProcessHandle(m_runner->pid()));

    const bool cppDebug = isCppDebugging();
    const bool qmlDebug = isQmlDebugging();
    if (cppDebug) {
        setInferiorExecutable(iosRunConfig->localExecutable().toString());
        setRemoteChannel("connect://localhost:" + gdbServerPort.toString());

        QString bundlePath = iosRunConfig->bundleDirectory().toString();
        bundlePath.chop(4);
        FileName dsymPath = FileName::fromString(bundlePath.append(".dSYM"));
        if (dsymPath.exists() && dsymPath.toFileInfo().lastModified()
                < QFileInfo(iosRunConfig->localExecutable().toUserOutput()).lastModified()) {
            TaskHub::addTask(Task::Warning,
                             tr("The dSYM %1 seems to be outdated, it might confuse the debugger.")
                             .arg(dsymPath.toUserOutput()),
                             ProjectExplorer::Constants::TASK_CATEGORY_DEPLOYMENT);
        }
    }

    QUrl qmlServer;
    if (qmlDebug) {
        QTcpServer server;
        QTC_ASSERT(server.listen(QHostAddress::LocalHost)
                   || server.listen(QHostAddress::LocalHostIPv6), return);
        qmlServer.setHost(server.serverAddress().toString());
        if (!cppDebug)
            setStartMode(AttachToRemoteServer);
    }

    if (qmlServerPort.isValid())
        qmlServer.setPort(qmlServerPort.number());

    setQmlServer(qmlServer);

    DebuggerRunTool::start();
}

} // namespace Internal
} // namespace Ios
