// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "cmakeprojectconstants.h"
#include "cmakespecificsettings.h"

#include <coreplugin/icore.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <utils/layoutbuilder.h>

using namespace Utils;

namespace CMakeProjectManager {
namespace Internal {

CMakeSpecificSettings::CMakeSpecificSettings()
{
    // TODO: fixup of QTCREATORBUG-26289 , remove in Qt Creator 7 or so
    Core::ICore::settings()->remove("CMakeSpecificSettings/NinjaPath");

    setSettingsGroup("CMakeSpecificSettings");
    setAutoApply(false);

    registerAspect(&afterAddFileSetting);
    afterAddFileSetting.setSettingsKey("ProjectPopupSetting");
    afterAddFileSetting.setDefaultValue(AfterAddFileAction::AskUser);
    afterAddFileSetting.addOption(tr("Ask about copying file paths"));
    afterAddFileSetting.addOption(tr("Do not copy file paths"));
    afterAddFileSetting.addOption(tr("Copy file paths"));
    afterAddFileSetting.setToolTip(tr("Determines whether file paths are copied "
        "to the clipboard for pasting to the CMakeLists.txt file when you "
        "add new files to CMake projects."));

    registerAspect(&ninjaPath);
    ninjaPath.setSettingsKey("NinjaPath");
    // never save this to the settings:
    ninjaPath.setToSettingsTransformation(
        [](const QVariant &) { return QVariant::fromValue(QString()); });

    registerAspect(&packageManagerAutoSetup);
    packageManagerAutoSetup.setSettingsKey("PackageManagerAutoSetup");
    packageManagerAutoSetup.setDefaultValue(false);
    packageManagerAutoSetup.setLabelText(tr("Package manager auto setup"));
    packageManagerAutoSetup.setToolTip(tr("Add the CMAKE_PROJECT_INCLUDE_BEFORE variable "
        "pointing to a CMake script that will install dependencies from the conanfile.txt, "
        "conanfile.py, or vcpkg.json file from the project source directory."));

    registerAspect(&askBeforeReConfigureInitialParams);
    askBeforeReConfigureInitialParams.setSettingsKey("AskReConfigureInitialParams");
    askBeforeReConfigureInitialParams.setDefaultValue(true);
    askBeforeReConfigureInitialParams.setLabelText(tr("Ask before re-configuring with "
        "initial parameters"));

    registerAspect(&showSourceSubFolders);
    showSourceSubFolders.setSettingsKey("ShowSourceSubFolders");
    showSourceSubFolders.setDefaultValue(true);
    showSourceSubFolders.setLabelText(tr("Show subfolders inside source group folders"));
}

// CMakeSpecificSettingsPage

CMakeSpecificSettingsPage::CMakeSpecificSettingsPage(CMakeSpecificSettings *settings)
{
    setId(Constants::Settings::GENERAL_ID);
    setDisplayName(tr("General"));
    setDisplayCategory("CMake");
    setCategory(Constants::Settings::CATEGORY);
    setCategoryIconPath(Constants::Icons::SETTINGS_CATEGORY);
    setSettings(settings);

    setLayouter([settings](QWidget *widget) {
        CMakeSpecificSettings &s = *settings;
        using namespace Layouting;
        Column {
            Group {
                title(::CMakeProjectManager::Internal::CMakeSpecificSettings::tr("Adding Files")),
                Column { s.afterAddFileSetting }
            },
            s.packageManagerAutoSetup,
            s.askBeforeReConfigureInitialParams,
            s.showSourceSubFolders,
            st
        }.attachTo(widget);
    });
}

} // Internal
} // CMakeProjectManager
