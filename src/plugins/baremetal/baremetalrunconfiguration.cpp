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
        executable.setDeviceSelector(target, ExecutableAspect::RunDevice);
        executable.setPlaceHolderText(Tr::tr("Unknown"));

        arguments.setMacroExpander(macroExpander());

        workingDir.setMacroExpander(macroExpander());

        setUpdater([this] {
            const BuildTargetInfo bti = buildTargetInfo();
            executable.setExecutable(bti.targetFilePath);
        });

        connect(target, &Target::buildSystemUpdated, this, &RunConfiguration::update);
    }

    ExecutableAspect executable{this};
    ArgumentsAspect arguments{this};
    WorkingDirectoryAspect workingDir{this};
};

class BareMetalCustomRunConfiguration final : public RunConfiguration
{
public:
    explicit BareMetalCustomRunConfiguration(Target *target, Id id)
        : RunConfiguration(target, id)
    {
        executable.setDeviceSelector(target, ExecutableAspect::RunDevice);
        executable.setSettingsKey("BareMetal.CustomRunConfig.Executable");
        executable.setPlaceHolderText(Tr::tr("Unknown"));
        executable.setReadOnly(false);
        executable.setHistoryCompleter("BareMetal.CustomRunConfig.History");
        executable.setExpectedKind(PathChooser::Any);

        arguments.setMacroExpander(macroExpander());

        workingDir.setMacroExpander(macroExpander());

        setDefaultDisplayName(RunConfigurationFactory::decoratedTargetName(
            Tr::tr("Custom Executable"), target));
    }

public:
    Tasks checkForIssues() const final
    {
        Tasks tasks;
        if (executable.executable().isEmpty()) {
            tasks << createConfigurationIssue(Tr::tr("The remote executable must be set in order to "
                                                     "run a custom remote run configuration."));
        }
        return tasks;
    }

    ExecutableAspect executable{this};
    ArgumentsAspect arguments{this};
    WorkingDirectoryAspect workingDir{this};
};


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
