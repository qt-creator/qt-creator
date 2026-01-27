// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "projectsettingswidget.h"

#include <coreplugin/icore.h>

#include <utils/layoutbuilder.h>

#include <QCheckBox>
#include <QLabel>

namespace ProjectExplorer {

const int CONTENTS_MARGIN = 5;

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

void ProjectSettingsWidget::setUseGlobalSettingsCheckBoxVisible(bool visible)
{
    m_useGlobalSettingsCheckBoxVisibleVisible = visible;
}

void ProjectSettingsWidget::setUseGlobalSettingsLabelVisible(bool visible)
{
    m_useGlobalSettingsLabelVisibleVisible = visible;
}

void ProjectSettingsWidget::setGlobalSettingsId(Utils::Id globalId)
{
    m_globalSettingsId = globalId;
}

void ProjectSettingsWidget::setExpanding(bool expanding)
{
    m_expanding = expanding;
}

void ProjectSettingsWidget::addGlobalOrProjectSelectorToLayout(QBoxLayout *layout)
{
    if (!m_useGlobalSettingsCheckBoxVisibleVisible && !m_useGlobalSettingsLabelVisibleVisible)
        return;

    const auto useGlobalSettingsCheckBox = new QCheckBox;
    useGlobalSettingsCheckBox->setChecked(useGlobalSettings());
    useGlobalSettingsCheckBox->setEnabled(m_useGlobalSettingsCheckBoxEnabled);

    const QString labelText = m_useGlobalSettingsCheckBoxVisibleVisible
            ? QStringLiteral("Use <a href=\"dummy\">global settings</a>")
            : QStringLiteral("<a href=\"dummy\">Global settings</a>");
    const auto settingsLabel = new QLabel(labelText);
    settingsLabel->setEnabled(m_useGlobalSettingsCheckBoxEnabled);

    const auto horizontalLayout = new QHBoxLayout;
    horizontalLayout->setContentsMargins(0, CONTENTS_MARGIN, 0, CONTENTS_MARGIN);
    horizontalLayout->setSpacing(CONTENTS_MARGIN);

    if (m_useGlobalSettingsCheckBoxVisibleVisible) {
        horizontalLayout->addWidget(useGlobalSettingsCheckBox);

        connect(this, &ProjectSettingsWidget::useGlobalSettingsCheckBoxEnabledChanged,
                this, [useGlobalSettingsCheckBox, settingsLabel](bool enabled) {
            useGlobalSettingsCheckBox->setEnabled(enabled);
            settingsLabel->setEnabled(enabled);
        });
        connect(useGlobalSettingsCheckBox, &QCheckBox::stateChanged,
                this, &ProjectSettingsWidget::setUseGlobalSettings);
        connect(this, &ProjectSettingsWidget::useGlobalSettingsChanged,
                useGlobalSettingsCheckBox, &QCheckBox::setChecked);
    }

    if (m_useGlobalSettingsLabelVisibleVisible) {
        horizontalLayout->addWidget(settingsLabel);
        connect(settingsLabel, &QLabel::linkActivated, this, [this] {
            Core::ICore::showSettings(m_globalSettingsId);
        });
    }
    horizontalLayout->addStretch(1);
    layout->addLayout(horizontalLayout);
    layout->addWidget(Layouting::createHr());
}

} // ProjectExplorer
