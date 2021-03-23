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

#include <coreplugin/icore.h>

#include <projectexplorer/projectexplorerconstants.h>

#include <utils/layoutbuilder.h>

using namespace Utils;

namespace CMakeProjectManager {
namespace Internal {

CMakeSpecificSettings::CMakeSpecificSettings()
{
    setSettingsGroup("CMakeSpecificSettings");
    setAutoApply(false);

    registerAspect(&m_afterAddFileToProjectSetting);
    m_afterAddFileToProjectSetting.setSettingsKey("ProjectPopupSetting");
    m_afterAddFileToProjectSetting.setDefaultValue(AfterAddFileAction::ASK_USER);
    m_afterAddFileToProjectSetting.addOption(tr("Ask about copying file paths"));
    m_afterAddFileToProjectSetting.addOption(tr("Do not copy file paths"));
    m_afterAddFileToProjectSetting.addOption(tr("Copy file paths"));
    m_afterAddFileToProjectSetting.setToolTip(tr("Determines whether file paths are copied "
        "to the clipboard for pasting to the CMakeLists.txt file when you "
        "add new files to CMake projects."));

    registerAspect(&m_ninjaPath);
    m_ninjaPath.setSettingsKey("NinjaPath");

    registerAspect(&m_packageManagerAutoSetup);
    m_packageManagerAutoSetup.setSettingsKey("PackageManagerAutoSetup");
    m_packageManagerAutoSetup.setDefaultValue(true);
    m_packageManagerAutoSetup.setLabelText(tr("Package manager auto setup"));
    m_packageManagerAutoSetup.setToolTip(tr("Add the CMAKE_PROJECT_INCLUDE_BEFORE variable "
        "pointing to a CMake script that will install dependencies from the conanfile.txt, "
        "conanfile.py, or vcpkg.json file from the project source directory."));

    registerAspect(&m_askBeforeReConfigureInitialParams);
    m_askBeforeReConfigureInitialParams.setSettingsKey("AskReConfigureInitialParams");
    m_askBeforeReConfigureInitialParams.setDefaultValue(true);
    m_askBeforeReConfigureInitialParams.setLabelText(tr("Ask before re-configuring with "
        "initial parameters"));
}

// CMakeSpecificSettingWidget

class CMakeSpecificSettingWidget final : public Core::IOptionsPageWidget
{
    Q_DECLARE_TR_FUNCTIONS(CMakeProjectManager::Internal::CMakeSpecificSettingWidget)

public:
    explicit CMakeSpecificSettingWidget(CMakeSpecificSettings *settings);

    void apply() final;

private:
    CMakeSpecificSettings *m_settings;
};

CMakeSpecificSettingWidget::CMakeSpecificSettingWidget(CMakeSpecificSettings *settings)
    : m_settings(settings)
{
    CMakeSpecificSettings &s = *m_settings;
    using namespace Layouting;

    Column {
        Group {
            Title(tr("Adding Files")),
            s.m_afterAddFileToProjectSetting
        },
        s.m_packageManagerAutoSetup,
        s.m_askBeforeReConfigureInitialParams,
        Stretch(),
    }.attachTo(this);
}

void CMakeSpecificSettingWidget::apply()
{
    if (m_settings->isDirty()) {
        m_settings->apply();
        m_settings->writeSettings(Core::ICore::settings());
    }
}

// CMakeSpecificSettingsPage

CMakeSpecificSettingsPage::CMakeSpecificSettingsPage(CMakeSpecificSettings *settings)
{
    setId("CMakeSpecificSettings");
    setDisplayName(CMakeSpecificSettingWidget::tr("CMake"));
    setCategory(ProjectExplorer::Constants::BUILD_AND_RUN_SETTINGS_CATEGORY);
    setWidgetCreator([settings] { return new CMakeSpecificSettingWidget(settings); });
}

} // Internal
} // CMakeProjectManager
