/****************************************************************************
**
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

#include "baremetaldebugsupport.h"
#include "baremetalrunconfiguration.h"
#include "baremetaldevice.h"
#include "baremetalgdbcommandsdeploystep.h"

#include "gdbserverprovider.h"
#include "gdbserverprovidermanager.h"

#include <debugger/debuggerruncontrol.h>
#include <debugger/debuggerkitinformation.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/project.h>
#include <projectexplorer/runnables.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>

#include <utils/portlist.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

using namespace Debugger;
using namespace ProjectExplorer;

namespace BareMetal {
namespace Internal {

BareMetalDebugSupport::BareMetalDebugSupport(RunControl *runControl)
    : Debugger::DebuggerRunTool(runControl)
{
    auto dev = qSharedPointerCast<const BareMetalDevice>(device());
    if (!dev) {
        reportFailure(tr("Cannot debug: Kit has no device."));
        return;
    }

    const QString providerId = dev->gdbServerProviderId();
    const GdbServerProvider *p = GdbServerProviderManager::findProvider(providerId);
    if (!p) {
        // FIXME: Translate.
        reportFailure(QString("No GDB server provider found for %1").arg(providerId));
        return;
    }

    if (p->startupMode() == GdbServerProvider::StartupOnNetwork) {
        StandardRunnable r;
        r.executable = p->executable();
        // We need to wrap the command arguments depending on a host OS,
        // as the bare metal's GDB servers are launched on a host,
        // but not on a target.
        r.commandLineArguments = Utils::QtcProcess::joinArgs(p->arguments(), Utils::HostOsInfo::hostOs());
        m_gdbServer = new SimpleTargetRunner(runControl);
        m_gdbServer->setRunnable(r);
        addStartDependency(m_gdbServer);
    }
}

void BareMetalDebugSupport::start()
{
    const auto rc = qobject_cast<BareMetalRunConfiguration *>(runControl()->runConfiguration());
    QTC_ASSERT(rc, reportFailure(); return);

    const QString bin = rc->localExecutableFilePath();
    if (bin.isEmpty()) {
        reportFailure(tr("Cannot debug: Local executable is not set."));
        return;
    }
    if (!QFile::exists(bin)) {
        reportFailure(tr("Cannot debug: Could not find executable for \"%1\".").arg(bin));
        return;
    }

    const Target *target = rc->target();
    QTC_ASSERT(target, reportFailure(); return);

    auto dev = qSharedPointerCast<const BareMetalDevice>(device());
    QTC_ASSERT(dev, reportFailure(); return);
    const GdbServerProvider *p = GdbServerProviderManager::findProvider(dev->gdbServerProviderId());
    QTC_ASSERT(p, reportFailure(); return);

#if 0
    // Currently baremetal plugin does not provide way to configure deployments steps
    // FIXME: Should it?
    QString commands;
    if (const BuildConfiguration *bc = target->activeBuildConfiguration()) {
        if (BuildStepList *bsl = bc->stepList(BareMetalGdbCommandsDeployStep::stepId())) {
            foreach (const BareMetalGdbCommandsDeployStep *bs, bsl->allOfType<BareMetalGdbCommandsDeployStep>()) {
                if (!commands.endsWith("\n"))
                    commands.append("\n");
                commands.append(bs->gdbCommands());
            }
        }
    }
    setCommandsAfterConnect(commands);
#endif

    StandardRunnable inferior;
    inferior.executable = bin;
    inferior.commandLineArguments = rc->arguments();
    setInferior(inferior);
    setSymbolFile(bin);
    setStartMode(AttachToRemoteServer);
    setCommandsAfterConnect(p->initCommands()); // .. and here?
    setCommandsForReset(p->resetCommands());
    setRemoteChannel(p->channel());
    setUseContinueInsteadOfRun(true);

    DebuggerRunTool::start();
}

} // namespace Internal
} // namespace BareMetal
