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

#include <coreplugin/icore.h>
#include <projectexplorer/projectexplorerconstants.h>

namespace CMakeProjectManager {
namespace Internal {

CMakeSpecificSettingWidget::CMakeSpecificSettingWidget(QWidget *parent):
    QWidget(parent)
{
    m_ui.setupUi(this);
    m_ui.newFileAddedCopyToCpliSettingGroup->setId(m_ui.alwaysAskRadio,
                                                   AfterAddFileAction::ASK_USER);
    m_ui.newFileAddedCopyToCpliSettingGroup->setId(m_ui.neverCopyRadio,
                                                   AfterAddFileAction::NEVER_COPY_FILE_PATH);
    m_ui.newFileAddedCopyToCpliSettingGroup->setId(m_ui.alwaysCopyRadio,
                                                   AfterAddFileAction::COPY_FILE_PATH);
}

void CMakeSpecificSettingWidget::setSettings(const CMakeSpecificSettings &settings)
{
    setProjectPopupSetting(settings.afterAddFileSetting());
}

CMakeSpecificSettings CMakeSpecificSettingWidget::settings() const
{
    CMakeSpecificSettings set;

    int popupSetting = m_ui.newFileAddedCopyToCpliSettingGroup->checkedId();
    set.setAfterAddFileSetting(popupSetting == -1 ? AfterAddFileAction::ASK_USER
                                                  : static_cast<AfterAddFileAction>(popupSetting));
    return set;
}

void CMakeSpecificSettingWidget::setProjectPopupSetting(AfterAddFileAction mode)
{
    switch (mode) {
    case CMakeProjectManager::Internal::ASK_USER:
        m_ui.alwaysAskRadio->setChecked(true);
        break;
    case CMakeProjectManager::Internal::COPY_FILE_PATH:
        m_ui.alwaysCopyRadio->setChecked(true);
        break;
    case CMakeProjectManager::Internal::NEVER_COPY_FILE_PATH:
        m_ui.neverCopyRadio->setChecked(true);
        break;
    }
}

CMakeSpecificSettingsPage::CMakeSpecificSettingsPage(CMakeSpecificSettings *settings,
                                                     QObject *parent):
    Core::IOptionsPage(parent), m_settings(settings)
{
    setId("CMakeSpecificSettings");
    setDisplayName(tr("CMake"));
    setCategory(ProjectExplorer::Constants::BUILD_AND_RUN_SETTINGS_CATEGORY);
}

QWidget *CMakeSpecificSettingsPage::widget()
{
    if (!m_widget) {
        m_widget = new CMakeSpecificSettingWidget();
        m_widget->setSettings(*m_settings);
    }
    return m_widget;
}

void CMakeSpecificSettingsPage::apply()
{
    if (!m_widget) // page was never shown
        return;
    const CMakeSpecificSettings newSettings = m_widget->settings();
    *m_settings = newSettings;
    m_settings->toSettings(Core::ICore::settings());
}

void CMakeSpecificSettingsPage::finish()
{
    delete m_widget;
    m_widget = nullptr;
}

} // Internal
} // CMakeProjectManager
