// Copyright (C) 2016 Denis Shienkov <denis.shienkov@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "baremetaldebugsupport.h"

#include "baremetalconstants.h"
#include "baremetaldevice.h"
#include "baremetaltr.h"

#include "debugserverproviderchooser.h"
#include "debugserverprovidermanager.h"
#include "idebugserverprovider.h"

#include <debugger/debuggerruncontrol.h>

#include <projectexplorer/environmentkitaspect.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <QtTaskTree/QBarrier>

#include <utils/layoutbuilder.h>
#include <utils/portlist.h>
#include <utils/qtcprocess.h>

using namespace Debugger;
using namespace ProjectExplorer;
using namespace QtTaskTree;
using namespace Utils;

namespace BareMetal {

DebugServerProviderAspect::DebugServerProviderAspect(AspectContainer *container)
    : StringAspect(container)
{
    setLabelText(Tr::tr("Debug server provider:"));
}

void DebugServerProviderAspect::addToLayoutImpl(Layouting::Layout &parent)
{
    auto chooser = createSubWidget<Internal::DebugServerProviderChooser>();
    chooser->populate();
    chooser->setCurrentProviderId(value());
    connect(chooser, &Internal::DebugServerProviderChooser::providerChanged, this, [this, chooser] {
        setValue(chooser->currentProviderId());
    });
    addLabeledItem(parent, chooser);
}

Group debugServerRecipe(RunControl *runControl, const QString &providerId)
{
    Internal::IDebugServerProvider *p = Internal::DebugServerProviderManager::findProvider(
        providerId);
    if (!p)
        return runControl->errorTask(Tr::tr("No debug server provider found for %1").arg(providerId));

    DebuggerRunParameters rp = DebuggerRunParameters::fromRunControl(runControl);
    if (Result<> res = p->setupDebuggerRunParameters(rp, runControl); !res)
        return runControl->errorTask(res.error());

    const std::optional<BarrierKickerGetter> serverRunner = p->serverRunner(runControl);
    if (!serverRunner)
        return debuggerRecipe(runControl, rp);

    return {
        When (*serverRunner, WorkflowPolicy::StopOnSuccessOrError) >> Do {
            debuggerRecipe(runControl, rp)
        }
    };
}

namespace Internal {

class BareMetalDebugSupportFactory final : public RunWorkerFactory
{
public:
    BareMetalDebugSupportFactory()
    {
        setId("BareMetalDebugSupportFactory");
        setRecipeProducer([](RunControl *runControl) -> Group {
            const auto dev = std::static_pointer_cast<const BareMetalDevice>(runControl->device());
            if (!dev)
                return runControl->errorTask(Tr::tr("Cannot debug: Kit has no device."));
            return debugServerRecipe(runControl, dev->debugServerProviderId());
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

} // Internal
} // BareMetal
