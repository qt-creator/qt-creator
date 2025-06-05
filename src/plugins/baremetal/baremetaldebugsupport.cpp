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

#include <solutions/tasking/barrier.h>

#include <utils/portlist.h>
#include <utils/qtcprocess.h>
#include <utils/qtcassert.h>

using namespace Debugger;
using namespace ProjectExplorer;
using namespace Tasking;
using namespace Utils;

namespace BareMetal::Internal {

class BareMetalDebugSupportFactory final : public RunWorkerFactory
{
public:
    BareMetalDebugSupportFactory()
    {
        setRecipeProducer([](RunControl *runControl) -> Group {
            const auto dev = std::static_pointer_cast<const BareMetalDevice>(runControl->device());
            if (!dev) {
                // TODO: reportFailure won't work from RunWorker's c'tor.
                runControl->postMessage(Tr::tr("Cannot debug: Kit has no device."), ErrorMessageFormat);
                return {};
            }

            const QString providerId = dev->debugServerProviderId();
            IDebugServerProvider *p = DebugServerProviderManager::findProvider(providerId);
            if (!p) {
                // TODO: reportFailure won't work from RunWorker's c'tor.
                runControl->postMessage(Tr::tr("No debug server provider found for %1").arg(providerId),
                                        ErrorMessageFormat);
                return {};
            }

            DebuggerRunParameters rp = DebuggerRunParameters::fromRunControl(runControl);
            if (Result<> res = p->setupDebuggerRunParameters(rp, runControl); !res) {
                runControl->postMessage(res.error(), ErrorMessageFormat); // TODO: reportFailure won't work from RunWorker's c'tor.
                return {};
            }
            const std::optional<ProcessTask> targetRunner = p->targetProcess(runControl);
            if (!targetRunner)
                return debuggerRecipe(runControl, rp);

            return {
                When (*targetRunner, &Process::started, WorkflowPolicy::StopOnSuccessOrError) >> Do {
                    debuggerRecipe(runControl, rp)
                }
            };
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
