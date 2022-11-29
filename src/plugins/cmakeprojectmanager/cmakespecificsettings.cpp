// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "cmakespecificsettings.h"

#include "cmakeprojectconstants.h"
#include "cmakeprojectmanagertr.h"

#include <coreplugin/icore.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <utils/layoutbuilder.h>

using namespace Utils;

namespace CMakeProjectManager::Internal {

CMakeSpecificSettings::CMakeSpecificSettings()
{
    // TODO: fixup of QTCREATORBUG-26289 , remove in Qt Creator 7 or so
    Core::ICore::settings()->remove("CMakeSpecificSettings/NinjaPath");

    setSettingsGroup("CMakeSpecificSettings");
    setAutoApply(false);

    registerAspect(&afterAddFileSetting);
    afterAddFileSetting.setSettingsKey("ProjectPopupSetting");
    afterAddFileSetting.setDefaultValue(AfterAddFileAction::AskUser);
    afterAddFileSetting.addOption(::CMakeProjectManager::Tr::tr("Ask about copying file paths"));
    afterAddFileSetting.addOption(::CMakeProjectManager::Tr::tr("Do not copy file paths"));
    afterAddFileSetting.addOption(::CMakeProjectManager::Tr::tr("Copy file paths"));
    afterAddFileSetting.setToolTip(::CMakeProjectManager::Tr::tr("Determines whether file paths are copied "
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
    packageManagerAutoSetup.setLabelText(::CMakeProjectManager::Tr::tr("Package manager auto setup"));
    packageManagerAutoSetup.setToolTip(::CMakeProjectManager::Tr::tr("Add the CMAKE_PROJECT_INCLUDE_BEFORE variable "
        "pointing to a CMake script that will install dependencies from the conanfile.txt, "
        "conanfile.py, or vcpkg.json file from the project source directory."));

    registerAspect(&askBeforeReConfigureInitialParams);
    askBeforeReConfigureInitialParams.setSettingsKey("AskReConfigureInitialParams");
    askBeforeReConfigureInitialParams.setDefaultValue(true);
    askBeforeReConfigureInitialParams.setLabelText(::CMakeProjectManager::Tr::tr("Ask before re-configuring with "
        "initial parameters"));

    registerAspect(&showSourceSubFolders);
    showSourceSubFolders.setSettingsKey("ShowSourceSubFolders");
    showSourceSubFolders.setDefaultValue(true);
    showSourceSubFolders.setLabelText(
                ::CMakeProjectManager::Tr::tr("Show subfolders inside source group folders"));

    registerAspect(&showAdvancedOptionsByDefault);
    showAdvancedOptionsByDefault.setSettingsKey("ShowAdvancedOptionsByDefault");
    showAdvancedOptionsByDefault.setDefaultValue(false);
    showAdvancedOptionsByDefault.setLabelText(
                ::CMakeProjectManager::Tr::tr("Show advanced options by default"));
}

// CMakeSpecificSettingsPage

CMakeSpecificSettingsPage::CMakeSpecificSettingsPage(CMakeSpecificSettings *settings)
{
    setId(Constants::Settings::GENERAL_ID);
    setDisplayName(::CMakeProjectManager::Tr::tr("General"));
    setDisplayCategory("CMake");
    setCategory(Constants::Settings::CATEGORY);
    setCategoryIconPath(Constants::Icons::SETTINGS_CATEGORY);
    setSettings(settings);

    setLayouter([settings](QWidget *widget) {
        CMakeSpecificSettings &s = *settings;
        using namespace Layouting;
        Column {
            Group {
                title(::CMakeProjectManager::Tr::tr("Adding Files")),
                Column { s.afterAddFileSetting }
            },
            s.packageManagerAutoSetup,
            s.askBeforeReConfigureInitialParams,
            s.showSourceSubFolders,
            s.showAdvancedOptionsByDefault,
            st
        }.attachTo(widget);
    });
}

} // CMakeProjectManager::Internal
