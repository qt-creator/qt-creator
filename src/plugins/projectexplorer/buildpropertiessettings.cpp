/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "buildpropertiessettings.h"

#include "projectexplorerconstants.h"

#include <utils/layoutbuilder.h>

using namespace Utils;

namespace ProjectExplorer {

// Default directory:
const char DEFAULT_BUILD_DIRECTORY_TEMPLATE[]
    = "../%{JS: Util.asciify(\"build-%{Project:Name}-%{Kit:FileSystemName}-%{BuildConfig:Name}\")}";

BuildPropertiesSettings::BuildTriStateAspect::BuildTriStateAspect()
    : TriStateAspect{
          BuildPropertiesSettings::tr("Enable"),
          BuildPropertiesSettings::tr("Disable"),
          BuildPropertiesSettings::tr("Use Project Default")}
{}

BuildPropertiesSettings::BuildPropertiesSettings()
{
    setAutoApply(false);

    registerAspect(&buildDirectoryTemplate);
    buildDirectoryTemplate.setDisplayStyle(StringAspect::LineEditDisplay);
    buildDirectoryTemplate.setSettingsKey("Directories/BuildDirectory.TemplateV2");
    buildDirectoryTemplate.setDefaultValue(DEFAULT_BUILD_DIRECTORY_TEMPLATE);
    buildDirectoryTemplate.setLabelText(tr("Default build directory:"));
    buildDirectoryTemplate.setUseGlobalMacroExpander();
    buildDirectoryTemplate.setUseResetButton();

    registerAspect(&buildDirectoryTemplateOld); // TODO: Remove in ~4.16
    buildDirectoryTemplateOld.setSettingsKey("Directories/BuildDirectory.Template");
    buildDirectoryTemplateOld.setDefaultValue(DEFAULT_BUILD_DIRECTORY_TEMPLATE);

    registerAspect(&separateDebugInfo);
    separateDebugInfo.setSettingsKey("ProjectExplorer/Settings/SeparateDebugInfo");
    separateDebugInfo.setLabelText(tr("Separate debug info:"));

    registerAspect(&qmlDebugging);
    qmlDebugging.setSettingsKey("ProjectExplorer/Settings/QmlDebugging");
    qmlDebugging.setLabelText(tr("QML debugging:"));

    registerAspect(&qtQuickCompiler);
    qtQuickCompiler.setSettingsKey("ProjectExplorer/Settings/QtQuickCompiler");
    qtQuickCompiler.setLabelText(tr("Use qmlcachegen:"));

    QObject::connect(&showQtSettings, &BoolAspect::valueChanged,
                     &qmlDebugging, &BaseAspect::setVisible);
    QObject::connect(&showQtSettings, &BoolAspect::valueChanged,
                     &qtQuickCompiler, &BaseAspect::setVisible);
}

void BuildPropertiesSettings::readSettings(QSettings *s)
{
    AspectContainer::readSettings(s);

    // TODO: Remove in ~4.16
    QString v = buildDirectoryTemplate.value();
    if (v.isEmpty())
        v = buildDirectoryTemplateOld.value();
    if (v.isEmpty())
        v = DEFAULT_BUILD_DIRECTORY_TEMPLATE;
    v.replace("%{CurrentProject:Name}", "%{Project:Name}");
    v.replace("%{CurrentKit:FileSystemName}", "%{Kit:FileSystemName}");
    v.replace("%{CurrentBuild:Name}", "%{BuildConfig:Name}");
    buildDirectoryTemplate.setValue(v);
}

QString BuildPropertiesSettings::defaultBuildDirectoryTemplate()
{
    return QString(DEFAULT_BUILD_DIRECTORY_TEMPLATE);
}

namespace Internal {

BuildPropertiesSettingsPage::BuildPropertiesSettingsPage(BuildPropertiesSettings *settings)
{
    setId("AB.ProjectExplorer.BuildPropertiesSettingsPage");
    setDisplayName(BuildPropertiesSettings::tr("Default Build Properties"));
    setCategory(ProjectExplorer::Constants::BUILD_AND_RUN_SETTINGS_CATEGORY);
    setSettings(settings);

    setLayouter([settings](QWidget *widget) {
        BuildPropertiesSettings &s = *settings;
        using namespace Layouting;

        Column {
            Form {
                s.buildDirectoryTemplate,
                s.separateDebugInfo,
                s.qmlDebugging,
                s.qtQuickCompiler
            },
            Stretch()
        }.attachTo(widget);
    });
}

} // Internal
} // ProjectExplorer
