/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "settingspage.h"
#include "settings.h"
#include "cpasterconstants.h"

#include <coreplugin/icore.h>

#include <QTextStream>
#include <QCoreApplication>

namespace CodePaster {

class SettingsWidget final : public Core::IOptionsPageWidget
{
public:
    SettingsWidget(const QStringList &protocols, Settings *settings);

private:
    void apply() final;

    Settings *m_settings;
    Internal::Ui::SettingsPage m_ui;
};

SettingsWidget::SettingsWidget(const QStringList &protocols, Settings *settings)
    : m_settings(settings)
{
    m_ui.setupUi(this);
    m_ui.defaultProtocol->addItems(protocols);

    m_ui.userEdit->setText(m_settings->username);
    const int index = m_ui.defaultProtocol->findText(m_settings->protocol);
    m_ui.defaultProtocol->setCurrentIndex(index == -1 ? 0  : index);
    m_ui.expirySpinBox->setValue(m_settings->expiryDays);
    m_ui.publicCheckBox->setChecked(m_settings->publicPaste);
    m_ui.clipboardBox->setChecked(m_settings->copyToClipboard);
    m_ui.displayBox->setChecked(m_settings->displayOutput);
}

void SettingsWidget::apply()
{
    Settings rc;
    rc.username = m_ui.userEdit->text();
    rc.protocol = m_ui.defaultProtocol->currentText();
    rc.expiryDays = m_ui.expirySpinBox->value();
    rc.publicPaste = m_ui.publicCheckBox->isChecked();
    rc.copyToClipboard = m_ui.clipboardBox->isChecked();
    rc.displayOutput = m_ui.displayBox->isChecked();

    if (rc != *m_settings) {
        *m_settings = rc;
        m_settings->toSettings(Core::ICore::settings());
    }
}

SettingsPage::SettingsPage(Settings *settings, const QStringList &protocolNames)
{
    setId("A.CodePaster.General");
    setDisplayName(tr("General"));
    setCategory(Constants::CPASTER_SETTINGS_CATEGORY);
    setDisplayCategory(QCoreApplication::translate("CodePaster", "Code Pasting"));
    setCategoryIconPath(":/cpaster/images/settingscategory_cpaster.png");
    setWidgetCreator([settings, protocolNames] {
        return new SettingsWidget(protocolNames, settings);
    });
}

} // namespace CodePaster
