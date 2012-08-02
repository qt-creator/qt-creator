/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (C) 2011 - 2012 Research In Motion
**
** Contact: Research In Motion (blackberry-qt@qnx.com)
** Contact: KDAB (info@kdab.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

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
#include <debugger/debuggerprofileinformation.h>
#include <debugger/debuggerstartparameters.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>
#include <qtsupport/qtprofileinformation.h>
#include <utils/portlist.h>

using namespace Debugger;
using namespace ProjectExplorer;
using namespace Qnx;
using namespace Qnx::Internal;

DebuggerStartParameters createStartParameters(const QnxRunConfiguration *runConfig)
{
    DebuggerStartParameters params;
    Target *target = runConfig->target();
    Profile *profile = target->profile();

    const IDevice::ConstPtr device = DeviceProfileInformation::device(profile);
    if (device.isNull())
        return params;

    params.startMode = AttachToRemoteServer;
    params.debuggerCommand = DebuggerProfileInformation::debuggerCommand(profile).toString();
    params.sysRoot = SysRootProfileInformation::sysRoot(profile).toString();

    if (ToolChain *tc = ToolChainProfileInformation::toolChain(profile))
        params.toolChainAbi = tc->targetAbi();

    params.symbolFileName = runConfig->localExecutableFilePath();
    params.remoteExecutable = runConfig->remoteExecutableFilePath();
    params.remoteChannel = device->sshParameters().host + QLatin1String(":-1");
    params.displayName = runConfig->displayName();
    params.remoteSetupNeeded = true;
    params.closeMode = DetachAtClose;

    QnxQtVersion *qtVersion =
            dynamic_cast<QnxQtVersion *>(QtSupport::QtProfileInformation::qtVersion(profile));
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
        const QnxDeviceConfiguration::ConstPtr dev = DeviceProfileInformation::device(runConfiguration->target()->profile())
                  .dynamicCast<const QnxDeviceConfiguration>();
        if (dev.isNull())
            return false;
        return rc->portsUsedByDebuggers() <= dev->freePorts().count();
    }
    return true;
}

RunControl *QnxRunControlFactory::create(RunConfiguration *runConfig, RunMode mode)
{
    Q_ASSERT(canRun(runConfig, mode));

    QnxRunConfiguration *rc = qobject_cast<QnxRunConfiguration *>(runConfig);
    Q_ASSERT(rc);
    if (mode == NormalRunMode)
        return new QnxRunControl(rc);

    const DebuggerStartParameters params = createStartParameters(rc);
    DebuggerRunControl * const runControl = DebuggerPlugin::createDebugger(params, rc);
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
