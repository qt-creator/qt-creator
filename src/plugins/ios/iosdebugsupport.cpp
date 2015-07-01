/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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
#include <qmakeprojectmanager/qmakebuildconfiguration.h>
#include <qmakeprojectmanager/qmakenodes.h>
#include <qmakeprojectmanager/qmakeproject.h>
#include <qtsupport/qtkitinformation.h>
#include <utils/fileutils.h>
#include <utils/qtcprocess.h>

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
using namespace QmakeProjectManager;

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
        params.executable = runConfig->localExecutable().toString();
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
    if (qmlDebug && !cppDebug) {
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
      m_runner(new IosRunner(this, runConfig, cppDebug, qmlDebug))
{
    connect(m_runControl, SIGNAL(requestRemoteSetup()),
            m_runner, SLOT(start()));
    connect(m_runControl, SIGNAL(finished()),
            m_runner, SLOT(stop()));

    connect(m_runner, SIGNAL(gotServerPorts(int,int)),
        SLOT(handleServerPorts(int,int)));
    connect(m_runner, SIGNAL(gotInferiorPid(Q_PID,int)),
        SLOT(handleGotInferiorPid(Q_PID,int)));
    connect(m_runner, SIGNAL(finished(bool)),
        SLOT(handleRemoteProcessFinished(bool)));

    connect(m_runner, SIGNAL(errorMsg(QString)),
        SLOT(handleRemoteErrorOutput(QString)));
    connect(m_runner, SIGNAL(appOutput(QString)),
        SLOT(handleRemoteOutput(QString)));
}

IosDebugSupport::~IosDebugSupport()
{
}

void IosDebugSupport::handleServerPorts(int gdbServerPort, int qmlPort)
{
    RemoteSetupResult result;
    result.gdbServerPort = gdbServerPort;
    result.qmlServerPort = qmlPort;
    result.success = gdbServerPort > 0 || (m_runner && !m_runner->cppDebug() && qmlPort > 0);
    if (!result.success)
        result.reason =  tr("Could not get debug server file descriptor.");
    m_runControl->notifyEngineRemoteSetupFinished(result);
}

void IosDebugSupport::handleGotInferiorPid(Q_PID pid, int qmlPort)
{
    RemoteSetupResult result;
    result.qmlServerPort = qmlPort;
    result.inferiorPid = int(Utils::qPidToPid(pid));
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
