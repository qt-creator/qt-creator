/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator Squish plugin.
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

#include "squishsettingspage.h"
#include "squishconstants.h"
#include "squishsettings.h"

#include <coreplugin/icore.h>

#include <utils/theme/theme.h>

namespace Squish {
namespace Internal {

SquishSettingsWidget::SquishSettingsWidget(QWidget *parent)
    : QWidget(parent)
{
    m_ui.setupUi(this);

    connect(m_ui.localCheckBox, &QCheckBox::toggled, this, &SquishSettingsWidget::onLocalToggled);
}

void SquishSettingsWidget::setSettings(const SquishSettings &settings)
{
    m_ui.squishPathChooser->setFilePath(settings.squishPath);
    m_ui.licensePathChooser->setFilePath(settings.licensePath);
    m_ui.localCheckBox->setChecked(settings.local);
    m_ui.serverHostLineEdit->setText(settings.serverHost);
    m_ui.serverPortSpinBox->setValue(settings.serverPort);
    m_ui.verboseCheckBox->setChecked(settings.verbose);
}

SquishSettings SquishSettingsWidget::settings() const
{
    SquishSettings result;
    result.squishPath = m_ui.squishPathChooser->filePath();
    result.licensePath = m_ui.licensePathChooser->filePath();
    result.local = m_ui.localCheckBox->checkState() == Qt::Checked;
    result.serverHost = m_ui.serverHostLineEdit->text();
    result.serverPort = m_ui.serverPortSpinBox->value();
    result.verbose = m_ui.verboseCheckBox->checkState() == Qt::Checked;
    return result;
}

void SquishSettingsWidget::onLocalToggled(bool checked)
{
    m_ui.serverHostLineEdit->setEnabled(!checked);
    m_ui.serverPortSpinBox->setEnabled(!checked);
}

SquishSettingsPage::SquishSettingsPage(const QSharedPointer<SquishSettings> &settings)
    : m_settings(settings)
    , m_widget(nullptr)
{
    setId("A.Squish.General");
    setDisplayName(tr("General"));
    setCategory(Constants::SQUISH_SETTINGS_CATEGORY);
    setDisplayCategory(tr("Squish"));
    setCategoryIcon(Utils::Icon({{":/squish/images/settingscategory_squish.png",
                                  Utils::Theme::PanelTextColorDark}},
                                Utils::Icon::Tint));
}

QWidget *SquishSettingsPage::widget()
{
    if (!m_widget) {
        m_widget = new SquishSettingsWidget;
        m_widget->setSettings(*m_settings);
    }
    return m_widget;
}

void SquishSettingsPage::apply()
{
    if (!m_widget) // page was not shown at all
        return;

    const SquishSettings newSettings = m_widget->settings();
    if (newSettings != *m_settings) {
        *m_settings = newSettings;
        m_settings->toSettings(Core::ICore::settings());
    }
}

} // namespace Internal
} // namespace Squish
