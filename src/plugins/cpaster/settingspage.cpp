/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "settingspage.h"
#include "settings.h"
#include "cpasterconstants.h"

#include <coreplugin/icore.h>

#include <QTextStream>
#include <QCoreApplication>

namespace CodePaster {

SettingsWidget::SettingsWidget(const QStringList &protocols, QWidget *parent) :
    QWidget(parent)
{
    m_ui.setupUi(this);
    m_ui.defaultProtocol->addItems(protocols);
}

void SettingsWidget::setSettings(const Settings &settings)
{
    m_ui.userEdit->setText(settings.username);
    const int index = m_ui.defaultProtocol->findText(settings.protocol);
    m_ui.defaultProtocol->setCurrentIndex(index == -1 ? 0  : index);
    m_ui.expirySpinBox->setValue(settings.expiryDays);
    m_ui.clipboardBox->setChecked(settings.copyToClipboard);
    m_ui.displayBox->setChecked(settings.displayOutput);
}

Settings SettingsWidget::settings()
{
    Settings rc;
    rc.username = m_ui.userEdit->text();
    rc.protocol = m_ui.defaultProtocol->currentText();
    rc.expiryDays = m_ui.expirySpinBox->value();
    rc.copyToClipboard = m_ui.clipboardBox->isChecked();
    rc.displayOutput = m_ui.displayBox->isChecked();
    return rc;
}

SettingsPage::SettingsPage(const QSharedPointer<Settings> &settings) :
    m_settings(settings), m_widget(0)
{
    setId("A.CodePaster.General");
    setDisplayName(tr("General"));
    setCategory(Constants::CPASTER_SETTINGS_CATEGORY);
    setDisplayCategory(QCoreApplication::translate("CodePaster",
        Constants::CPASTER_SETTINGS_TR_CATEGORY));
    setCategoryIcon(QLatin1String(Constants::SETTINGS_CATEGORY_CPASTER_ICON));
}

SettingsPage::~SettingsPage()
{
}

QWidget *SettingsPage::widget()
{
    if (!m_widget) {
        m_widget = new SettingsWidget(m_protocols);
        m_widget->setSettings(*m_settings);
    }
    return m_widget;
}

void SettingsPage::apply()
{
    if (!m_widget) // page was never shown
        return;
    const Settings newSettings = m_widget->settings();
    if (newSettings != *m_settings) {
        *m_settings = newSettings;
        m_settings->toSettings(Core::ICore::settings());
    }
}

void SettingsPage::addProtocol(const QString &name)
{
    m_protocols.append(name);
}

} // namespace CodePaster
