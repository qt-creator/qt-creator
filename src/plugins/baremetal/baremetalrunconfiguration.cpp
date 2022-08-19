// Copyright (C) 2016 Tim Sander <tim@krieglstein.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "baremetalconstants.h"
#include "baremetalrunconfiguration.h"

#include <projectexplorer/buildsystem.h>
#include <projectexplorer/buildtargetinfo.h>
#include <projectexplorer/project.h>
#include <projectexplorer/runconfigurationaspects.h>
#include <projectexplorer/target.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace BareMetal {
namespace Internal {

// RunConfigurations

class BareMetalRunConfiguration final : public RunConfiguration
{
    Q_DECLARE_TR_FUNCTIONS(BareMetal::Internal::BareMetalRunConfiguration)

public:
    explicit BareMetalRunConfiguration(Target *target, Id id)
        : RunConfiguration(target, id)
    {
        const auto exeAspect = addAspect<ExecutableAspect>(target, ExecutableAspect::RunDevice);
        exeAspect->setDisplayStyle(StringAspect::LabelDisplay);
        exeAspect->setPlaceHolderText(tr("Unknown"));

        addAspect<ArgumentsAspect>(macroExpander());
        addAspect<WorkingDirectoryAspect>(macroExpander(), nullptr);

        setUpdater([this, exeAspect] {
            const BuildTargetInfo bti = buildTargetInfo();
            exeAspect->setExecutable(bti.targetFilePath);
        });

        connect(target, &Target::buildSystemUpdated, this, &RunConfiguration::update);
    }
};

class BareMetalCustomRunConfiguration final : public RunConfiguration
{
    Q_DECLARE_TR_FUNCTIONS(BareMetal::Internal::BareMetalCustomRunConfiguration)

public:
    explicit BareMetalCustomRunConfiguration(Target *target, Id id)
        : RunConfiguration(target, id)
    {
        const auto exeAspect = addAspect<ExecutableAspect>(target, ExecutableAspect::RunDevice);
        exeAspect->setSettingsKey("BareMetal.CustomRunConfig.Executable");
        exeAspect->setPlaceHolderText(tr("Unknown"));
        exeAspect->setDisplayStyle(StringAspect::PathChooserDisplay);
        exeAspect->setHistoryCompleter("BareMetal.CustomRunConfig.History");
        exeAspect->setExpectedKind(PathChooser::Any);

        addAspect<ArgumentsAspect>(macroExpander());
        addAspect<WorkingDirectoryAspect>(macroExpander(), nullptr);

        setDefaultDisplayName(RunConfigurationFactory::decoratedTargetName(tr("Custom Executable"), target));
    }

public:
    Tasks checkForIssues() const final;
};

Tasks BareMetalCustomRunConfiguration::checkForIssues() const
{
    Tasks tasks;
    if (aspect<ExecutableAspect>()->executable().isEmpty()) {
        tasks << createConfigurationIssue(tr("The remote executable must be set in order to run "
                                             "a custom remote run configuration."));
    }
    return tasks;
}

// BareMetalRunConfigurationFactory

BareMetalRunConfigurationFactory::BareMetalRunConfigurationFactory()
{
    registerRunConfiguration<BareMetalRunConfiguration>("BareMetalCustom");
    setDecorateDisplayNames(true);
    addSupportedTargetDeviceType(BareMetal::Constants::BareMetalOsType);
}

// BaseMetalCustomRunConfigurationFactory

BareMetalCustomRunConfigurationFactory::BareMetalCustomRunConfigurationFactory()
    : FixedRunConfigurationFactory(BareMetalCustomRunConfiguration::tr("Custom Executable"), true)
{
    registerRunConfiguration<BareMetalCustomRunConfiguration>("BareMetal");
    addSupportedTargetDeviceType(BareMetal::Constants::BareMetalOsType);
}

} // namespace Internal
} // namespace BareMetal

