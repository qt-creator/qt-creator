// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "projectsettingswidget.h"

#include <coreplugin/icore.h>

#include <utils/layoutbuilder.h>

#include <QCheckBox>
#include <QLabel>

namespace ProjectExplorer {

const int CONTENTS_MARGIN = 5;

QWidget *createGlobalSettingsLink(Utils::Id globalId)
{
    const auto label = new QLabel(R"(<a href="dummy">Global settings</a>)");
    QObject::connect(label, &QLabel::linkActivated, label, [globalId] {
        Core::ICore::showSettings(globalId);
    });
    using namespace Layouting;
    return Column {
        Row { label, st, customMargins(0, CONTENTS_MARGIN, 0, CONTENTS_MARGIN), spacing(CONTENTS_MARGIN) },
        createHr(),
        noMargin, spacing(0),
    }.emerge();
}

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

QWidget *ProjectSettingsWidget::createGlobalOrProjectSelector()
{
    const auto checkBox = new QCheckBox;
    checkBox->setChecked(useGlobalSettings());
    checkBox->setEnabled(m_useGlobalSettingsCheckBoxEnabled);

    const auto label = new QLabel(
        QStringLiteral("Use <a href=\"dummy\">global settings</a>"));
    label->setEnabled(m_useGlobalSettingsCheckBoxEnabled);

    connect(this, &ProjectSettingsWidget::useGlobalSettingsCheckBoxEnabledChanged,
            this, [checkBox, label](bool enabled) {
        checkBox->setEnabled(enabled);
        label->setEnabled(enabled);
    });
    connect(checkBox, &QCheckBox::stateChanged,
            this, &ProjectSettingsWidget::setUseGlobalSettings);
    connect(this, &ProjectSettingsWidget::useGlobalSettingsChanged,
            checkBox, &QCheckBox::setChecked);
    connect(label, &QLabel::linkActivated, this, [this] {
        Core::ICore::showSettings(m_globalSettingsId);
    });

    using namespace Layouting;
    return Column {
        Row { checkBox, label, st, customMargins(0, CONTENTS_MARGIN, 0, CONTENTS_MARGIN), spacing(CONTENTS_MARGIN) },
        createHr(),
        noMargin, spacing(0),
    }.emerge();
}

} // ProjectExplorer
