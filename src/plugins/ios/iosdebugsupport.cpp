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

#include "iosdebugsupport.h"

#include "iosrunner.h"
#include "iosmanager.h"
#include "iosdevice.h"

#include <debugger/debuggerplugin.h>
#include <debugger/debuggerkitinformation.h>
#include <debugger/debuggerruncontrol.h>
#include <debugger/debuggerstartparameters.h>
#include <debugger/debuggerrunconfigurationaspect.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/target.h>
#include <projectexplorer/taskhub.h>
#include <qtsupport/qtkitinformation.h>
#include <utils/fileutils.h>
#include <utils/qtcprocess.h>

#include <QDateTime>
#include <QDir>
#include <QTcpServer>
#include <QSettings>

#include <stdio.h>
#include <fcntl.h>
#ifdef Q_OS_UNIX
#include <unistd.h>
#else
#include <io.h>
#endif

using namespace Debugger;
using namespace ProjectExplorer;

namespace Ios {
namespace Internal {

RunControl *IosDebugSupport::createDebugRunControl(IosRunConfiguration *runConfig,
                                                   QString *errorMessage)
{
    Target *target = runConfig->target();
    if (!target)
        return 0;
    IDevice::ConstPtr device = DeviceKitInformation::device(target->kit());
    if (device.isNull())
        return 0;

    DebuggerStartParameters params;
    if (device->type() == Core::Id(Ios::Constants::IOS_DEVICE_TYPE)) {
        params.startMode = AttachToRemoteProcess;
        params.platform = QLatin1String("remote-ios");
        IosDevice::ConstPtr iosDevice = device.dynamicCast<const IosDevice>();
        if (iosDevice.isNull())
                return 0;
        QString osVersion = iosDevice->osVersion();
        Utils::FileName deviceSdk1 = Utils::FileName::fromString(QDir::homePath()
                                             + QLatin1String("/Library/Developer/Xcode/iOS DeviceSupport/")
                                             + osVersion + QLatin1String("/Symbols"));
        QString deviceSdk;
        if (deviceSdk1.toFileInfo().isDir()) {
            deviceSdk = deviceSdk1.toString();
        } else {
            Utils::FileName deviceSdk2 = IosConfigurations::developerPath()
                    .appendPath(QLatin1String("Platforms/iPhoneOS.platform/DeviceSupport/"))
                    .appendPath(osVersion).appendPath(QLatin1String("Symbols"));
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
        params.deviceSymbolsRoot = deviceSdk;
    } else {
        params.startMode = AttachExternal;
        params.platform = QLatin1String("ios-simulator");
    }
    params.displayName = runConfig->applicationName();
    params.remoteSetupNeeded = true;
    params.continueAfterAttach = true;

    auto aspect = runConfig->extraAspect<DebuggerRunConfigurationAspect>();
    bool cppDebug = aspect->useCppDebugger();
    bool qmlDebug = aspect->useQmlDebugger();
    if (cppDebug) {
        params.inferior.executable = runConfig->localExecutable().toString();
        params.remoteChannel = QLatin1String("connect://localhost:0");

        Utils::FileName xcodeInfo = IosConfigurations::developerPath().parentDir()
                .appendPath(QLatin1String("Info.plist"));
        bool buggyLldb = false;
        if (xcodeInfo.exists()) {
            QSettings settings(xcodeInfo.toString(), QSettings::NativeFormat);
            QStringList version = settings.value(QLatin1String("CFBundleShortVersionString")).toString()
                    .split(QLatin1Char('.'));
            if (version.value(0).toInt() == 5 && version.value(1, QString::number(1)).toInt() == 0)
                buggyLldb = true;
        }
        QString bundlePath = runConfig->bundleDirectory().toString();
        bundlePath.chop(4);
        Utils::FileName dsymPath = Utils::FileName::fromString(
                    bundlePath.append(QLatin1String(".dSYM")));
        if (!dsymPath.exists()) {
            if (buggyLldb)
                TaskHub::addTask(Task::Warning,
                                 tr("Debugging with Xcode 5.0.x can be unreliable without a dSYM. "
                                    "To create one, add a dsymutil deploystep."),
                                 ProjectExplorer::Constants::TASK_CATEGORY_DEPLOYMENT);
        } else if (dsymPath.toFileInfo().lastModified()
                   < QFileInfo(runConfig->localExecutable().toUserOutput()).lastModified()) {
            TaskHub::addTask(Task::Warning,
                             tr("The dSYM %1 seems to be outdated, it might confuse the debugger.")
                             .arg(dsymPath.toUserOutput()),
                             ProjectExplorer::Constants::TASK_CATEGORY_DEPLOYMENT);
        }
    }

    if (qmlDebug) {
        QTcpServer server;
        QTC_ASSERT(server.listen(QHostAddress::LocalHost)
                   || server.listen(QHostAddress::LocalHostIPv6), return 0);
        params.qmlServer.host = server.serverAddress().toString();
        if (!cppDebug)
            params.startMode = AttachToRemoteServer;
    }

    DebuggerRunControl *debuggerRunControl = createDebuggerRunControl(params, runConfig, errorMessage);
    if (debuggerRunControl)
        new IosDebugSupport(runConfig, debuggerRunControl, cppDebug, qmlDebug);
    return debuggerRunControl;
}

IosDebugSupport::IosDebugSupport(IosRunConfiguration *runConfig,
    DebuggerRunControl *runControl, bool cppDebug, bool qmlDebug)
    : QObject(runControl), m_runControl(runControl),
      m_runner(new IosRunner(this, runConfig, cppDebug, qmlDebug ? QmlDebug::QmlDebuggerServices :
                                                                   QmlDebug::NoQmlDebugServices))
{
    connect(m_runControl, &DebuggerRunControl::requestRemoteSetup,
            m_runner, &IosRunner::start);
    connect(m_runControl, &RunControl::finished,
            m_runner, &IosRunner::stop);

    connect(m_runner, &IosRunner::gotServerPorts,
        this, &IosDebugSupport::handleServerPorts);
    connect(m_runner, &IosRunner::gotInferiorPid,
        this, &IosDebugSupport::handleGotInferiorPid);
    connect(m_runner, &IosRunner::finished,
        this, &IosDebugSupport::handleRemoteProcessFinished);

    connect(m_runner, &IosRunner::errorMsg,
        this, &IosDebugSupport::handleRemoteErrorOutput);
    connect(m_runner, &IosRunner::appOutput,
        this, &IosDebugSupport::handleRemoteOutput);
}

IosDebugSupport::~IosDebugSupport()
{
}

void IosDebugSupport::handleServerPorts(Utils::Port gdbServerPort, Utils::Port qmlPort)
{
    RemoteSetupResult result;
    result.gdbServerPort = gdbServerPort;
    result.qmlServerPort = qmlPort;
    result.success = gdbServerPort.isValid()
            || (m_runner && !m_runner->cppDebug() && qmlPort.isValid());
    if (!result.success)
        result.reason =  tr("Could not get debug server file descriptor.");
    m_runControl->notifyEngineRemoteSetupFinished(result);
}

void IosDebugSupport::handleGotInferiorPid(qint64 pid, Utils::Port qmlPort)
{
    RemoteSetupResult result;
    result.qmlServerPort = qmlPort;
    result.inferiorPid = pid;
    result.success = pid > 0;
    if (!result.success)
        result.reason =  tr("Got an invalid process id.");
    m_runControl->notifyEngineRemoteSetupFinished(result);
}

void IosDebugSupport::handleRemoteProcessFinished(bool cleanEnd)
{
    if (m_runControl) {
        if (!cleanEnd)
            m_runControl->appendMessage(tr("Run ended with error."), Utils::DebugFormat);
        else
            m_runControl->appendMessage(tr("Run ended."), Utils::DebugFormat);
        m_runControl->abortDebugger();
    }
}

void IosDebugSupport::handleRemoteOutput(const QString &output)
{
    if (m_runControl)
        m_runControl->showMessage(output, AppOutput);
}

void IosDebugSupport::handleRemoteErrorOutput(const QString &output)
{
    if (m_runControl)
        m_runControl->showMessage(output, AppError);
}

} // namespace Internal
} // namespace Ios
