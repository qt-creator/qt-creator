/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

#include "projectsettingswidget.h"

namespace ProjectExplorer {

ProjectSettingsWidget::ProjectSettingsWidget(QWidget *parent)
    : QWidget(parent)
{}

void ProjectSettingsWidget::setUseGlobalSettings(bool useGlobalSettings)
{
    if (m_useGlobalSettings == useGlobalSettings)
        return;
    m_useGlobalSettings = useGlobalSettings;
    emit useGlobalSettingsChanged(useGlobalSettings);
}

bool ProjectSettingsWidget::useGlobalSettings() const
{
    return m_useGlobalSettings;
}

void ProjectSettingsWidget::setUseGlobalSettingsCheckBoxEnabled(bool enabled)
{
    if (m_useGlobalSettingsCheckBoxEnabled == enabled)
        return;
    m_useGlobalSettingsCheckBoxEnabled = enabled;
    emit useGlobalSettingsCheckBoxEnabledChanged(enabled);
}

bool ProjectSettingsWidget::isUseGlobalSettingsCheckBoxEnabled() const
{
    return m_useGlobalSettingsCheckBoxEnabled;
}

bool ProjectSettingsWidget::isUseGlobalSettingsCheckBoxVisible() const
{
    return m_useGlobalSettingsCheckBoxVisibleVisible;
}

void ProjectSettingsWidget::setUseGlobalSettingsCheckBoxVisible(bool visible)
{
    m_useGlobalSettingsCheckBoxVisibleVisible = visible;
}

Utils::Id ProjectSettingsWidget::globalSettingsId() const
{
    return m_globalSettingsId;
}

void ProjectSettingsWidget::setGlobalSettingsId(Utils::Id globalId)
{
    m_globalSettingsId = globalId;
}

} // ProjectExplorer
