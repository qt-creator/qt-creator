/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "remotelinuxruncontrolfactory.h"

#include "remotelinuxdebugsupport.h"
#include "remotelinuxrunconfiguration.h"
#include "remotelinuxruncontrol.h"

#include <debugger/debuggerconstants.h>
#include <debugger/debuggerplugin.h>
#include <debugger/debuggerrunner.h>
#include <debugger/debuggerstartparameters.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <utils/portlist.h>

using namespace Debugger;
using namespace ProjectExplorer;

namespace RemoteLinux {
namespace Internal {

RemoteLinuxRunControlFactory::RemoteLinuxRunControlFactory(QObject *parent)
    : IRunControlFactory(parent)
{
}

RemoteLinuxRunControlFactory::~RemoteLinuxRunControlFactory()
{
}

bool RemoteLinuxRunControlFactory::canRun(RunConfiguration *runConfiguration, RunMode mode) const
{
    if (mode != NormalRunMode && mode != DebugRunMode && mode != DebugRunModeWithBreakOnMain)
        return false;

    const QString idStr = QString::fromLatin1(runConfiguration->id().name());
    if (!runConfiguration->isEnabled() || !idStr.startsWith(RemoteLinuxRunConfiguration::IdPrefix))
        return false;

    if (mode == NormalRunMode)
        return true;

    const RemoteLinuxRunConfiguration * const remoteRunConfig
            = qobject_cast<RemoteLinuxRunConfiguration *>(runConfiguration);
    if (mode == DebugRunMode) {
        IDevice::ConstPtr dev = DeviceKitInformation::device(runConfiguration->target()->kit());
        if (dev.isNull())
            return false;
        return remoteRunConfig->portsUsedByDebuggers() <= dev->freePorts().count();
    }
    return true;
}

RunControl *RemoteLinuxRunControlFactory::create(RunConfiguration *runConfig, RunMode mode, QString *errorMessage)
{
    Q_ASSERT(canRun(runConfig, mode));

    RemoteLinuxRunConfiguration *rc = qobject_cast<RemoteLinuxRunConfiguration *>(runConfig);
    Q_ASSERT(rc);
    if (mode == ProjectExplorer::NormalRunMode)
        return new RemoteLinuxRunControl(rc);

    DebuggerStartParameters params = LinuxDeviceDebugSupport::startParameters(rc);
    if (mode == ProjectExplorer::DebugRunModeWithBreakOnMain)
        params.breakOnMain = true;
    DebuggerRunControl * const runControl = DebuggerPlugin::createDebugger(params, rc, errorMessage);
    if (!runControl)
        return 0;
    LinuxDeviceDebugSupport * const debugSupport =
            new LinuxDeviceDebugSupport(rc, runControl->engine());
    connect(runControl, SIGNAL(finished()), debugSupport, SLOT(handleDebuggingFinished()));
    return runControl;
}

QString RemoteLinuxRunControlFactory::displayName() const
{
    return tr("Run on remote Linux device");
}

} // namespace Internal
} // namespace RemoteLinux
