// Copyright (C) 2016 Denis Shienkov <denis.shienkov@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "baremetaldebugsupport.h"

#include "baremetalconstants.h"
#include "baremetaldevice.h"
#include "baremetaltr.h"

#include "debugserverprovidermanager.h"
#include "idebugserverprovider.h"

#include <debugger/debuggerkitaspect.h>
#include <debugger/debuggerruncontrol.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/kitaspects.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/runconfigurationaspects.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>

#include <utils/portlist.h>
#include <utils/process.h>
#include <utils/qtcassert.h>

using namespace Debugger;
using namespace ProjectExplorer;
using namespace Utils;

namespace BareMetal::Internal {

class BareMetalDebugSupport final : public Debugger::DebuggerRunTool
{
public:
    explicit BareMetalDebugSupport(ProjectExplorer::RunControl *runControl)
        : Debugger::DebuggerRunTool(runControl)
    {
        const auto dev = qSharedPointerCast<const BareMetalDevice>(device());
        if (!dev) {
            reportFailure(Tr::tr("Cannot debug: Kit has no device."));
            return;
        }

        const QString providerId = dev->debugServerProviderId();
        IDebugServerProvider *p = DebugServerProviderManager::findProvider(providerId);
        if (!p) {
            reportFailure(Tr::tr("No debug server provider found for %1").arg(providerId));
            return;
        }

        if (RunWorker *runner = p->targetRunner(runControl))
            addStartDependency(runner);
    }

private:
    void start() final
    {
        const auto dev = qSharedPointerCast<const BareMetalDevice>(device());
        QTC_ASSERT(dev, reportFailure(); return);
        IDebugServerProvider *p = DebugServerProviderManager::findProvider(
            dev->debugServerProviderId());
        QTC_ASSERT(p, reportFailure(); return);

        QString errorMessage;
        if (!p->aboutToRun(this, errorMessage))
            reportFailure(errorMessage);
        else
            DebuggerRunTool::start();
    }
};

BareMetalDebugSupportFactory::BareMetalDebugSupportFactory()
{
    setProduct<BareMetalDebugSupport>();
    addSupportedRunMode(ProjectExplorer::Constants::NORMAL_RUN_MODE);
    addSupportedRunMode(ProjectExplorer::Constants::DEBUG_RUN_MODE);
    addSupportedRunConfig(BareMetal::Constants::BAREMETAL_RUNCONFIG_ID);
    addSupportedRunConfig(BareMetal::Constants::BAREMETAL_CUSTOMRUNCONFIG_ID);
}

} // BareMetal::Internal
