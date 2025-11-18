// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "customexecutablerunconfiguration.h"

#include "projectexplorerconstants.h"
#include "projectexplorertr.h"
#include "runconfigurationaspects.h"
#include "target.h"

using namespace Utils;

namespace ProjectExplorer {

// CustomExecutableRunConfiguration

CustomExecutableRunConfiguration::CustomExecutableRunConfiguration(BuildConfiguration *bc)
    : CustomExecutableRunConfiguration(bc, Constants::CUSTOM_EXECUTABLE_RUNCONFIG_ID)
{}

CustomExecutableRunConfiguration::CustomExecutableRunConfiguration(BuildConfiguration *bc, Id id)
    : RunConfiguration(bc, id)
{
    environment.setSupportForBuildEnvironment(bc);

    executable.setDeviceSelector(kit(), ExecutableAspect::HostDevice);
    executable.setSettingsKey("ProjectExplorer.CustomExecutableRunConfiguration.Executable");
    executable.setReadOnly(false);
    executable.setHistoryCompleter("Qt.CustomExecutable.History");
    executable.setExpectedKind(PathChooser::ExistingCommand);
    executable.setEnvironment(environment.environment());

    workingDir.setEnvironment(&environment);

    connect(&environment, &EnvironmentAspect::environmentChanged, this, [this]  {
         executable.setEnvironment(environment.environment());
    });

    setDefaultDisplayName(defaultDisplayName());
    setUsesEmptyBuildKeys();
}

bool CustomExecutableRunConfiguration::isEnabled(Id) const
{
    return true;
}

QString CustomExecutableRunConfiguration::defaultDisplayName() const
{
    if (executable().isEmpty())
        return Tr::tr("Custom Executable");
    return Tr::tr("Run %1").arg(executable().toUserOutput());
}

Tasks CustomExecutableRunConfiguration::checkForIssues() const
{
    Tasks tasks;
    if (executable().isEmpty()) {
        const QString summary = Tr::tr(
            "No executable configured in the custom run "
            "configuration.");
        const QString linkText = displayName();
        //: %1 = display name of the run configuration
        const QString detail = Tr::tr("Go to the %1 run configuration and set up an executable.");
        const QString link = Constants::URL_HANDLER_SCHEME + QChar(':')
                             + Constants::ACTIVE_RUN_CONFIG_PATH;

        QTextCharFormat format;
        format.setAnchor(true);
        format.setAnchorHref(link);
        const int offset = summary.length() + detail.indexOf("%1") + 1;
        const QTextLayout::FormatRange formatRange{offset, int(linkText.size()), format};

        Task task = createConfigurationIssue(summary);
        task.addToDetails(detail.arg(linkText));
        task.setFormats({formatRange});
        tasks << task;
    }
    return tasks;
}

// Factories

CustomExecutableRunConfigurationFactory::CustomExecutableRunConfigurationFactory() :
    FixedRunConfigurationFactory(Tr::tr("Custom Executable"))
{
    registerRunConfiguration<CustomExecutableRunConfiguration>(
        Constants::CUSTOM_EXECUTABLE_RUNCONFIG_ID);
}

} // namespace ProjectExplorer
