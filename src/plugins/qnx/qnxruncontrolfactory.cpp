/**************************************************************************
**
** Copyright (C) 2011 - 2013 Research In Motion
**
** Contact: Research In Motion (blackberry-qt@qnx.com)
** Contact: KDAB (info@kdab.com)
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

#include "qnxruncontrolfactory.h"
#include "qnxconstants.h"
#include "qnxrunconfiguration.h"
#include "qnxdebugsupport.h"
#include "qnxqtversion.h"
#include "qnxruncontrol.h"
#include "qnxutils.h"
#include "qnxdeviceconfiguration.h"

#include <debugger/debuggerengine.h>
#include <debugger/debuggerplugin.h>
#include <debugger/debuggerrunner.h>
#include <debugger/debuggerstartparameters.h>
#include <debugger/debuggerkitinformation.h>
#include <debugger/debuggerstartparameters.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>
#include <qtsupport/qtkitinformation.h>
#include <utils/portlist.h>

using namespace Debugger;
using namespace ProjectExplorer;
using namespace Qnx;
using namespace Qnx::Internal;

DebuggerStartParameters createStartParameters(const QnxRunConfiguration *runConfig)
{
    DebuggerStartParameters params;
    Target *target = runConfig->target();
    Kit *k = target->kit();

    const IDevice::ConstPtr device = DeviceKitInformation::device(k);
    if (device.isNull())
        return params;

    params.startMode = AttachToRemoteServer;
    params.debuggerCommand = DebuggerKitInformation::debuggerCommand(k).toString();
    params.sysRoot = SysRootKitInformation::sysRoot(k).toString();

    if (ToolChain *tc = ToolChainKitInformation::toolChain(k))
        params.toolChainAbi = tc->targetAbi();

    params.symbolFileName = runConfig->localExecutableFilePath();
    params.remoteExecutable = runConfig->remoteExecutableFilePath();
    params.remoteChannel = device->sshParameters().host + QLatin1String(":-1");
    params.displayName = runConfig->displayName();
    params.remoteSetupNeeded = true;
    params.closeMode = DetachAtClose;

    QnxQtVersion *qtVersion =
            dynamic_cast<QnxQtVersion *>(QtSupport::QtKitInformation::qtVersion(k));
    if (qtVersion)
        params.solibSearchPath = QnxUtils::searchPaths(qtVersion);

    return params;
}


QnxRunControlFactory::QnxRunControlFactory(QObject *parent)
    : IRunControlFactory(parent)
{
}

bool QnxRunControlFactory::canRun(RunConfiguration *runConfiguration, RunMode mode) const
{
    if (mode != NormalRunMode && mode != DebugRunMode)
        return false;

    if (!runConfiguration->isEnabled()
            || !runConfiguration->id().toString().startsWith(QLatin1String(Constants::QNX_QNX_RUNCONFIGURATION_PREFIX))) {
        return false;
    }


    const QnxRunConfiguration * const rc = qobject_cast<QnxRunConfiguration *>(runConfiguration);
    if (mode == DebugRunMode) {
        const QnxDeviceConfiguration::ConstPtr dev = DeviceKitInformation::device(runConfiguration->target()->kit())
                  .dynamicCast<const QnxDeviceConfiguration>();
        if (dev.isNull())
            return false;
        return rc->portsUsedByDebuggers() <= dev->freePorts().count();
    }
    return true;
}

RunControl *QnxRunControlFactory::create(RunConfiguration *runConfig, RunMode mode, QString *errorMessage)
{
    Q_ASSERT(canRun(runConfig, mode));

    QnxRunConfiguration *rc = qobject_cast<QnxRunConfiguration *>(runConfig);
    Q_ASSERT(rc);
    if (mode == NormalRunMode)
        return new QnxRunControl(rc);

    const DebuggerStartParameters params = createStartParameters(rc);
    DebuggerRunControl * const runControl = DebuggerPlugin::createDebugger(params, rc, errorMessage);
    if (!runControl)
        return 0;

    QnxDebugSupport *debugSupport = new QnxDebugSupport(rc, runControl->engine());
    connect(runControl, SIGNAL(finished()), debugSupport, SLOT(handleDebuggingFinished()));

    return runControl;
}

QString QnxRunControlFactory::displayName() const
{
    return tr("Run on remote QNX device");
}

RunConfigWidget *QnxRunControlFactory::createConfigurationWidget(RunConfiguration *config)
{
    Q_UNUSED(config)
    return 0;
}
