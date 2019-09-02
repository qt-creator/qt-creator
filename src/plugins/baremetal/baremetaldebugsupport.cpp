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
#include "baremetaldevice.h"

#include "gdbserverprovider.h"
#include "gdbserverprovidermanager.h"

#include <debugger/debuggerkitinformation.h>
#include <debugger/debuggerruncontrol.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/project.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/runconfigurationaspects.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>

#include <utils/portlist.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

using namespace Debugger;
using namespace ProjectExplorer;
using namespace Utils;

namespace BareMetal {
namespace Internal {

// BareMetalDebugSupport

class BareMetalGdbServer : public SimpleTargetRunner
{
public:
    BareMetalGdbServer(RunControl *runControl, const Runnable &runnable)
        : SimpleTargetRunner(runControl)
    {
        setId("BareMetalGdbServer");
        // Baremetal's GDB servers are launched on the host, not on the target.
        setStarter([this, runnable] { doStart(runnable, {}); });
    }
};

BareMetalDebugSupport::BareMetalDebugSupport(RunControl *runControl)
    : Debugger::DebuggerRunTool(runControl)
{
    const auto dev = qSharedPointerCast<const BareMetalDevice>(device());
    if (!dev) {
        reportFailure(tr("Cannot debug: Kit has no device."));
        return;
    }

    const QString providerId = dev->gdbServerProviderId();
    const GdbServerProvider *p = GdbServerProviderManager::findProvider(providerId);
    if (!p) {
        reportFailure(tr("No GDB server provider found for %1").arg(providerId));
        return;
    }

    if (p->startupMode() == GdbServerProvider::StartupOnNetwork) {
        Runnable r;
        r.setCommandLine(p->command());
        // Command arguments are in host OS style as the bare metal's GDB servers are launched
        // on the host, not on that target.
        auto gdbServer = new BareMetalGdbServer(runControl, r);
        addStartDependency(gdbServer);
    }
}

void BareMetalDebugSupport::start()
{
    const auto exeAspect = runControl()->aspect<ExecutableAspect>();
    QTC_ASSERT(exeAspect, reportFailure(); return);

    const FilePath bin = exeAspect->executable();
    if (bin.isEmpty()) {
        reportFailure(tr("Cannot debug: Local executable is not set."));
        return;
    }
    if (!bin.exists()) {
        reportFailure(tr("Cannot debug: Could not find executable for \"%1\".").arg(bin.toString()));
        return;
    }

    const auto dev = qSharedPointerCast<const BareMetalDevice>(device());
    QTC_ASSERT(dev, reportFailure(); return);
    const GdbServerProvider *p = GdbServerProviderManager::findProvider(dev->gdbServerProviderId());
    QTC_ASSERT(p, reportFailure(); return);

    Runnable inferior;
    inferior.executable = bin;
    if (const auto aspect = runControl()->aspect<ArgumentsAspect>())
        inferior.commandLineArguments = aspect->arguments(runControl()->macroExpander());
    setInferior(inferior);
    setSymbolFile(bin);
    setStartMode(AttachToRemoteServer);
    setCommandsAfterConnect(p->initCommands()); // .. and here?
    setCommandsForReset(p->resetCommands());
    setRemoteChannel(p->channelString());
    setUseContinueInsteadOfRun(true);
    setUseExtendedRemote(p->useExtendedRemote());

    DebuggerRunTool::start();
}

} // namespace Internal
} // namespace BareMetal
