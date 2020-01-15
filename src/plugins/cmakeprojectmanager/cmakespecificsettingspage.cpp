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

#include "cmakespecificsettingspage.h"
#include "cmakespecificsettings.h"
#include "ui_cmakespecificsettingspage.h"

#include <coreplugin/icore.h>
#include <projectexplorer/projectexplorerconstants.h>

namespace CMakeProjectManager {
namespace Internal {

class CMakeSpecificSettingWidget final : public Core::IOptionsPageWidget
{
    Q_DECLARE_TR_FUNCTIONS(CMakeProjectManager::Internal::CMakeSpecificSettingWidget)

public:
    explicit CMakeSpecificSettingWidget(CMakeSpecificSettings *settings);

    void apply() final;

private:
    Ui::CMakeSpecificSettingForm m_ui;
    CMakeSpecificSettings *m_settings;
};

CMakeSpecificSettingWidget::CMakeSpecificSettingWidget(CMakeSpecificSettings *settings)
    : m_settings(settings)
{
    m_ui.setupUi(this);
    m_ui.newFileAddedCopyToCpliSettingGroup->setId(m_ui.alwaysAskRadio,
                                                   AfterAddFileAction::ASK_USER);
    m_ui.newFileAddedCopyToCpliSettingGroup->setId(m_ui.neverCopyRadio,
                                                   AfterAddFileAction::NEVER_COPY_FILE_PATH);
    m_ui.newFileAddedCopyToCpliSettingGroup->setId(m_ui.alwaysCopyRadio,
                                                   AfterAddFileAction::COPY_FILE_PATH);

    const AfterAddFileAction mode = settings->afterAddFileSetting();
    switch (mode) {
    case ASK_USER:
        m_ui.alwaysAskRadio->setChecked(true);
        break;
    case COPY_FILE_PATH:
        m_ui.alwaysCopyRadio->setChecked(true);
        break;
    case NEVER_COPY_FILE_PATH:
        m_ui.neverCopyRadio->setChecked(true);
        break;
    }
}

void CMakeSpecificSettingWidget::apply()
{
    int popupSetting = m_ui.newFileAddedCopyToCpliSettingGroup->checkedId();
    m_settings->setAfterAddFileSetting(popupSetting == -1 ? AfterAddFileAction::ASK_USER
                                                  : static_cast<AfterAddFileAction>(popupSetting));
    m_settings->toSettings(Core::ICore::settings());
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
