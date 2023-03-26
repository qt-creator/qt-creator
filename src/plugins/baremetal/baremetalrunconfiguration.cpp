// Copyright (C) 2016 Tim Sander <tim@krieglstein.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "baremetalrunconfiguration.h"

#include "baremetalconstants.h"
#include "baremetaltr.h"

#include <projectexplorer/buildsystem.h>
#include <projectexplorer/buildtargetinfo.h>
#include <projectexplorer/project.h>
#include <projectexplorer/runconfigurationaspects.h>
#include <projectexplorer/target.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace BareMetal::Internal {

// RunConfigurations

class BareMetalRunConfiguration final : public RunConfiguration
{
public:
    explicit BareMetalRunConfiguration(Target *target, Id id)
        : RunConfiguration(target, id)
    {
        const auto exeAspect = addAspect<ExecutableAspect>(target, ExecutableAspect::RunDevice);
        exeAspect->setDisplayStyle(StringAspect::LabelDisplay);
        exeAspect->setPlaceHolderText(Tr::tr("Unknown"));

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
public:
    explicit BareMetalCustomRunConfiguration(Target *target, Id id)
        : RunConfiguration(target, id)
    {
        const auto exeAspect = addAspect<ExecutableAspect>(target, ExecutableAspect::RunDevice);
        exeAspect->setSettingsKey("BareMetal.CustomRunConfig.Executable");
        exeAspect->setPlaceHolderText(Tr::tr("Unknown"));
        exeAspect->setDisplayStyle(StringAspect::PathChooserDisplay);
        exeAspect->setHistoryCompleter("BareMetal.CustomRunConfig.History");
        exeAspect->setExpectedKind(PathChooser::Any);

        addAspect<ArgumentsAspect>(macroExpander());
        addAspect<WorkingDirectoryAspect>(macroExpander(), nullptr);

        setDefaultDisplayName(RunConfigurationFactory::decoratedTargetName(
                                  Tr::tr("Custom Executable"), target));
    }

public:
    Tasks checkForIssues() const final;
};

Tasks BareMetalCustomRunConfiguration::checkForIssues() const
{
    Tasks tasks;
    if (aspect<ExecutableAspect>()->executable().isEmpty()) {
        tasks << createConfigurationIssue(Tr::tr("The remote executable must be set in order to "
                                                 "run a custom remote run configuration."));
    }
    return tasks;
}

// BareMetalRunConfigurationFactory

BareMetalRunConfigurationFactory::BareMetalRunConfigurationFactory()
{
    registerRunConfiguration<BareMetalRunConfiguration>(Constants::BAREMETAL_RUNCONFIG_ID);
    setDecorateDisplayNames(true);
    addSupportedTargetDeviceType(BareMetal::Constants::BareMetalOsType);
}

// BaseMetalCustomRunConfigurationFactory

BareMetalCustomRunConfigurationFactory::BareMetalCustomRunConfigurationFactory()
    : FixedRunConfigurationFactory(Tr::tr("Custom Executable"), true)
{
    registerRunConfiguration<BareMetalCustomRunConfiguration>(Constants::BAREMETAL_CUSTOMRUNCONFIG_ID);
    addSupportedTargetDeviceType(BareMetal::Constants::BareMetalOsType);
}

} // BareMetal::Internal
