/****************************************************************************
**
** Copyright (C) 2016 Tim Sander <tim@krieglstein.org>
** Copyright (C) 2016 Denis Shienkov <denis.shienkov@gmail.com>
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

#include "baremetalruncontrolfactory.h"
#include "baremetalgdbcommandsdeploystep.h"
#include "baremetalrunconfiguration.h"
#include "baremetaldevice.h"
#include "baremetaldebugsupport.h"

#include "gdbserverprovider.h"
#include "gdbserverprovidermanager.h"

#include <debugger/debuggerruncontrol.h>
#include <debugger/debuggerstartparameters.h>
#include <debugger/debuggerkitinformation.h>
#include <debugger/analyzer/analyzerstartparameters.h>
#include <debugger/analyzer/analyzermanager.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/project.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>

#include <utils/portlist.h>
#include <utils/qtcassert.h>

#include <QApplication>

using namespace Debugger;
using namespace ProjectExplorer;

namespace BareMetal {
namespace Internal {

BareMetalRunControlFactory::BareMetalRunControlFactory(QObject *parent) :
    IRunControlFactory(parent)
{
}

bool BareMetalRunControlFactory::canRun(RunConfiguration *runConfiguration, Core::Id mode) const
{
    if (mode != ProjectExplorer::Constants::NORMAL_RUN_MODE
            && mode != ProjectExplorer::Constants::DEBUG_RUN_MODE
            && mode != ProjectExplorer::Constants::DEBUG_RUN_MODE_WITH_BREAK_ON_MAIN) {
        return false;
    }

    const QByteArray idStr = runConfiguration->id().name();
    return runConfiguration->isEnabled() && idStr.startsWith(BareMetalRunConfiguration::IdPrefix);
}

RunControl *BareMetalRunControlFactory::create(
        RunConfiguration *runConfiguration, Core::Id mode, QString *errorMessage)
{
    QTC_ASSERT(canRun(runConfiguration, mode), return 0);

    const auto rc = qobject_cast<BareMetalRunConfiguration *>(runConfiguration);
    QTC_ASSERT(rc, return 0);

    const QString bin = rc->localExecutableFilePath();
    if (bin.isEmpty()) {
        *errorMessage = tr("Cannot debug: Local executable is not set.");
        return 0;
    } else if (!QFile::exists(bin)) {
        *errorMessage = tr("Cannot debug: Could not find executable for \"%1\".")
                .arg(bin);
        return 0;
    }

    const Target *target = rc->target();
    QTC_ASSERT(target, return 0);

    const Kit *kit = target->kit();
    QTC_ASSERT(kit, return 0);

    auto dev = qSharedPointerCast<const BareMetalDevice>(DeviceKitInformation::device(kit));
    if (!dev) {
        *errorMessage = tr("Cannot debug: Kit has no device.");
        return 0;
    }

    const GdbServerProvider *p = GdbServerProviderManager::findProvider(dev->gdbServerProviderId());
    if (!p) {
        *errorMessage = tr("Cannot debug: Device has no GDB server provider configuration.");
        return 0;
    }

    DebuggerStartParameters sp;

    if (const BuildConfiguration *bc = target->activeBuildConfiguration()) {
        if (BuildStepList *bsl = bc->stepList(BareMetalGdbCommandsDeployStep::stepId())) {
            foreach (const BareMetalGdbCommandsDeployStep *bs, bsl->allOfType<BareMetalGdbCommandsDeployStep>()) {
                if (!sp.commandsAfterConnect.endsWith("\n"))
                    sp.commandsAfterConnect.append("\n");
                sp.commandsAfterConnect.append(bs->gdbCommands());
            }
        }
    }

    sp.inferior.executable = bin;
    sp.inferior.commandLineArguments = rc->arguments();
    sp.symbolFile = bin;
    sp.startMode = AttachToRemoteServer;
    sp.commandsAfterConnect = p->initCommands();
    sp.commandsForReset = p->resetCommands();
    sp.remoteChannel = p->channel();
    sp.useContinueInsteadOfRun = true;

    if (p->startupMode() == GdbServerProvider::StartupOnNetwork)
        sp.remoteSetupNeeded = true;

    DebuggerRunControl *runControl = createDebuggerRunControl(sp, rc, errorMessage, mode);
    if (runControl && sp.remoteSetupNeeded)
        new BareMetalDebugSupport(runControl);

    return runControl;
}

} // namespace Internal
} // namespace BareMetal
