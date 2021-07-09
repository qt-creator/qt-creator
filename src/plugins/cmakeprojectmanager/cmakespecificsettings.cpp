/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "cmakespecificsettings.h"

#include <projectexplorer/projectexplorerconstants.h>

#include <utils/layoutbuilder.h>

using namespace Utils;

namespace CMakeProjectManager {
namespace Internal {

CMakeSpecificSettings::CMakeSpecificSettings()
{
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

    registerAspect(&packageManagerAutoSetup);
    packageManagerAutoSetup.setSettingsKey("PackageManagerAutoSetup");
    packageManagerAutoSetup.setDefaultValue(true);
    packageManagerAutoSetup.setLabelText(tr("Package manager auto setup"));
    packageManagerAutoSetup.setToolTip(tr("Add the CMAKE_PROJECT_INCLUDE_BEFORE variable "
        "pointing to a CMake script that will install dependencies from the conanfile.txt, "
        "conanfile.py, or vcpkg.json file from the project source directory."));

    registerAspect(&askBeforeReConfigureInitialParams);
    askBeforeReConfigureInitialParams.setSettingsKey("AskReConfigureInitialParams");
    askBeforeReConfigureInitialParams.setDefaultValue(true);
    askBeforeReConfigureInitialParams.setLabelText(tr("Ask before re-configuring with "
        "initial parameters"));
}

// CMakeSpecificSettingsPage

CMakeSpecificSettingsPage::CMakeSpecificSettingsPage(CMakeSpecificSettings *settings)
{
    setId("CMakeSpecificSettings");
    setDisplayName(::CMakeProjectManager::Internal::CMakeSpecificSettings::tr("CMake"));
    setCategory(ProjectExplorer::Constants::BUILD_AND_RUN_SETTINGS_CATEGORY);
    setSettings(settings);

    setLayouter([settings](QWidget *widget) {
        CMakeSpecificSettings &s = *settings;
        using namespace Layouting;
        Column {
            Group {
                Title(::CMakeProjectManager::Internal::CMakeSpecificSettings::tr("Adding Files")),
                s.afterAddFileSetting
            },
            s.packageManagerAutoSetup,
            s.askBeforeReConfigureInitialParams,
            Stretch(),
        }.attachTo(widget);
    });
}

} // Internal
} // CMakeProjectManager
