// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cmakespecificsettings.h"

#include "cmakeprojectconstants.h"
#include "cmakeprojectmanagertr.h"

#include <coreplugin/icore.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <utils/layoutbuilder.h>

using namespace Utils;

namespace CMakeProjectManager::Internal {

static CMakeSpecificSettings *theSettings;

CMakeSpecificSettings *CMakeSpecificSettings::instance()
{
    return theSettings;
}

CMakeSpecificSettings::CMakeSpecificSettings()
{
    theSettings = this;

    setId(Constants::Settings::GENERAL_ID);
    setDisplayName(::CMakeProjectManager::Tr::tr("General"));
    setDisplayCategory("CMake");
    setCategory(Constants::Settings::CATEGORY);
    setCategoryIconPath(Constants::Icons::SETTINGS_CATEGORY);

    setLayouter([this](QWidget *widget) {
        using namespace Layouting;
        Column {
            autorunCMake,
            packageManagerAutoSetup,
            askBeforeReConfigureInitialParams,
            askBeforePresetsReload,
            showSourceSubFolders,
            showAdvancedOptionsByDefault,
            st
        }.attachTo(widget);
    });

    // TODO: fixup of QTCREATORBUG-26289 , remove in Qt Creator 7 or so
    Core::ICore::settings()->remove("CMakeSpecificSettings/NinjaPath");

    setSettingsGroup("CMakeSpecificSettings");
    setAutoApply(false);

    registerAspect(&autorunCMake);
    autorunCMake.setSettingsKey("AutorunCMake");
    autorunCMake.setDefaultValue(true);
    autorunCMake.setLabelText(::CMakeProjectManager::Tr::tr("Autorun CMake"));
    autorunCMake.setToolTip(::CMakeProjectManager::Tr::tr(
        "Automatically run CMake after changes to CMake project files."));

    registerAspect(&ninjaPath);
    ninjaPath.setSettingsKey("NinjaPath");
    // never save this to the settings:
    ninjaPath.setToSettingsTransformation(
        [](const QVariant &) { return QVariant::fromValue(QString()); });

    registerAspect(&packageManagerAutoSetup);
    packageManagerAutoSetup.setSettingsKey("PackageManagerAutoSetup");
    packageManagerAutoSetup.setDefaultValue(true);
    packageManagerAutoSetup.setLabelText(::CMakeProjectManager::Tr::tr("Package manager auto setup"));
    packageManagerAutoSetup.setToolTip(::CMakeProjectManager::Tr::tr("Add the CMAKE_PROJECT_INCLUDE_BEFORE variable "
        "pointing to a CMake script that will install dependencies from the conanfile.txt, "
        "conanfile.py, or vcpkg.json file from the project source directory."));

    registerAspect(&askBeforeReConfigureInitialParams);
    askBeforeReConfigureInitialParams.setSettingsKey("AskReConfigureInitialParams");
    askBeforeReConfigureInitialParams.setDefaultValue(true);
    askBeforeReConfigureInitialParams.setLabelText(::CMakeProjectManager::Tr::tr("Ask before re-configuring with "
        "initial parameters"));

    registerAspect(&askBeforePresetsReload);
    askBeforePresetsReload.setSettingsKey("AskBeforePresetsReload");
    askBeforePresetsReload.setDefaultValue(true);
    askBeforePresetsReload.setLabelText(::CMakeProjectManager::Tr::tr("Ask before reloading CMake Presets"));

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

    readSettings();
}

} // CMakeProjectManager::Internal
