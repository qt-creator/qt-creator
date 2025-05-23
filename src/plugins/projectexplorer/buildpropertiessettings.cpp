// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "buildpropertiessettings.h"

#include "buildconfiguration.h"
#include "projectexplorerconstants.h"
#include "projectexplorertr.h"
#include "runconfiguration.h"

#include <coreplugin/dialogs/ioptionspage.h>

#include <utils/environment.h>
#include <utils/layoutbuilder.h>

using namespace Utils;

namespace ProjectExplorer {

static QString defaultBuildDirectoryTemplate()
{
    return qtcEnvironmentVariable(
        Constants::QTC_DEFAULT_BUILD_DIRECTORY_TEMPLATE,
        "./build/%{Asciify:%{Kit:FileSystemName}-%{BuildConfig:Name}}");
}

static QString defaultWorkingDirectoryTemplate()
{
    return qtcEnvironmentVariable(
        Constants::QTC_DEFAULT_WORKING_DIRECTORY_TEMPLATE,
        "%{RunConfig:Executable:Path}");
}

BuildPropertiesSettings &buildPropertiesSettings()
{
    static BuildPropertiesSettings theSettings;
    return theSettings;
}

BuildPropertiesSettings::BuildTriStateAspect::BuildTriStateAspect(AspectContainer *container)
    : TriStateAspect(container, Tr::tr("Enable"), Tr::tr("Disable"), Tr::tr("Use Project Default"))
{}

BuildPropertiesSettings::BuildPropertiesSettings()
{
    setAutoApply(false);

    setLayouter([this] {
        using namespace Layouting;

        return Column {
            Form {
                buildDirectoryTemplate, br,
                workingDirectoryTemplate, br,
                separateDebugInfo, br,
                qmlDebugging, br,
                qtQuickCompiler
            },
            st
        };
    });

    buildDirectoryTemplate.setDisplayStyle(StringAspect::LineEditDisplay);
    buildDirectoryTemplate.setSettingsKey("Directories/BuildDirectory.TemplateV2");
    buildDirectoryTemplate.setDefaultValue(defaultBuildDirectoryTemplate());
    buildDirectoryTemplate.setLabelText(Tr::tr("Default build directory:"));
    buildDirectoryTemplate.setToolTip(
        Tr::tr("Template used to construct the default build directory.<br><br>"
               "The default value can be set using the environment variable "
               "<tt>%1</tt>.")
            .arg(Constants::QTC_DEFAULT_BUILD_DIRECTORY_TEMPLATE));
    buildDirectoryTemplate.setUseResetButton();
    BuildConfiguration::setupBuildDirMacroExpander(
        *buildDirectoryTemplate.macroExpander(), {}, {}, {}, {}, {}, {}, true);

    workingDirectoryTemplate.setDisplayStyle(StringAspect::LineEditDisplay);
    workingDirectoryTemplate.setSettingsKey("Directories/WorkingDirectory.Template");
    workingDirectoryTemplate.setDefaultValue(defaultWorkingDirectoryTemplate());
    workingDirectoryTemplate.setLabelText(Tr::tr("Default working directory:"));
    workingDirectoryTemplate.setToolTip(
        Tr::tr(
            "Template used to construct the default working directory of a run "
            "configuration.<br><br>"
            "The default value can be set using the environment variable <tt>%1</tt>.")
            .arg(Constants::QTC_DEFAULT_WORKING_DIRECTORY_TEMPLATE));
    workingDirectoryTemplate.setUseResetButton();
    MacroExpander &wdExp = *workingDirectoryTemplate.macroExpander();
    BuildConfiguration::setupBuildDirMacroExpander(wdExp, {}, {}, {}, {}, {}, {}, true);
    RunConfiguration::setupMacroExpander(wdExp, nullptr, true);

    separateDebugInfo.setSettingsKey("ProjectExplorer/Settings/SeparateDebugInfo");
    separateDebugInfo.setLabelText(Tr::tr("Separate debug info:"));

    qmlDebugging.setSettingsKey("ProjectExplorer/Settings/QmlDebugging");
    qmlDebugging.setLabelText(Tr::tr("QML debugging:"));
    qmlDebugging.setVisible(false);

    qtQuickCompiler.setSettingsKey("ProjectExplorer/Settings/QtQuickCompiler");
    qtQuickCompiler.setLabelText(Tr::tr("Use qmlcachegen:"));
    qtQuickCompiler.setVisible(false);

    readSettings();
}

void BuildPropertiesSettings::showQtSettings()
{
    buildPropertiesSettings().qmlDebugging.setVisible(true);
    buildPropertiesSettings().qtQuickCompiler.setVisible(true);
}

// BuildPropertiesSettingsPage

class BuildPropertiesSettingsPage final : public Core::IOptionsPage
{
public:
    BuildPropertiesSettingsPage()
    {
        setId("AB.ProjectExplorer.BuildPropertiesSettingsPage");
        setDisplayName(Tr::tr("Default Build Properties"));
        setCategory(ProjectExplorer::Constants::BUILD_AND_RUN_SETTINGS_CATEGORY);
        setSettingsProvider([] { return &buildPropertiesSettings(); });
    }
};

const BuildPropertiesSettingsPage settingsPage;

} // ProjectExplorer
