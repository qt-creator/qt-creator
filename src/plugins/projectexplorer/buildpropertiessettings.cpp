// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "buildpropertiessettings.h"

#include "projectexplorerconstants.h"
#include "projectexplorertr.h"

#include <coreplugin/dialogs/ioptionspage.h>

#include <utils/layoutbuilder.h>

using namespace Utils;

namespace ProjectExplorer {

static QString defaultBuildDirectoryTemplate()
{
    return "../%{JS: Util.asciify(\"build-%{Project:Name}-%{Kit:FileSystemName}-%{BuildConfig:Name}\")}";
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
    buildDirectoryTemplate.setUseGlobalMacroExpander();
    buildDirectoryTemplate.setUseResetButton();

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
