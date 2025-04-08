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
#include <projectexplorer/environmentkitaspect.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/runconfigurationaspects.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>

#include <utils/portlist.h>
#include <utils/qtcprocess.h>
#include <utils/qtcassert.h>

using namespace Debugger;
using namespace ProjectExplorer;
using namespace Utils;

namespace BareMetal::Internal {

class BareMetalDebugSupportFactory final : public RunWorkerFactory
{
public:
    BareMetalDebugSupportFactory()
    {
        setProducer([](RunControl *runControl) {
            DebuggerRunTool *debugger = new DebuggerRunTool(runControl);
            const auto dev = std::static_pointer_cast<const BareMetalDevice>(runControl->device());
            if (!dev) {
                // TODO: reportFailure won't work from RunWorker's c'tor.
                debugger->reportFailure(Tr::tr("Cannot debug: Kit has no device."));
                return debugger;
            }

            const QString providerId = dev->debugServerProviderId();
            IDebugServerProvider *p = DebugServerProviderManager::findProvider(providerId);
            if (!p) {
                // TODO: reportFailure won't work from RunWorker's c'tor.
                debugger->reportFailure(Tr::tr("No debug server provider found for %1").arg(providerId));
                return debugger;
            }

            if (RunWorker *runner = p->targetRunner(runControl))
                debugger->addStartDependency(runner);

            if (Result<> res = p->setupDebuggerRunParameters(debugger->runParameters(), runControl); !res)
                debugger->reportFailure(res.error()); // TODO: reportFailure won't work from RunWorker's c'tor.

            return debugger;
        });
        addSupportedRunMode(ProjectExplorer::Constants::NORMAL_RUN_MODE);
        addSupportedRunMode(ProjectExplorer::Constants::DEBUG_RUN_MODE);
        addSupportedRunConfig(BareMetal::Constants::BAREMETAL_RUNCONFIG_ID);
        addSupportedRunConfig(BareMetal::Constants::BAREMETAL_CUSTOMRUNCONFIG_ID);
    }
};

void setupBareMetalDebugSupport()
{
    static BareMetalDebugSupportFactory theBareMetalDebugSupportFactory;
}

} // BareMetal::Internal
