// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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

bool ProjectSettingsWidget::isUseGlobalSettingsLabelVisible() const
{
    return m_useGlobalSettingsLabelVisibleVisible;
}

void ProjectSettingsWidget::setUseGlobalSettingsLabelVisible(bool visible)
{
    m_useGlobalSettingsLabelVisibleVisible = visible;
}

Utils::Id ProjectSettingsWidget::globalSettingsId() const
{
    return m_globalSettingsId;
}

void ProjectSettingsWidget::setGlobalSettingsId(Utils::Id globalId)
{
    m_globalSettingsId = globalId;
}

bool ProjectSettingsWidget::expanding() const
{
    return m_expanding;
}

void ProjectSettingsWidget::setExpanding(bool expanding)
{
    m_expanding = expanding;
}

} // ProjectExplorer
