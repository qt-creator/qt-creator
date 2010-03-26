/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "settingspage.h"
#include "settings.h"
#include "cpasterconstants.h"

#include <utils/qtcassert.h>
#include <coreplugin/icore.h>

#include <QtCore/QSettings>
#include <QtCore/QTextStream>
#include <QtCore/QCoreApplication>

namespace CodePaster {

SettingsWidget::SettingsWidget(const QStringList &protocols, QWidget *parent) :
    QWidget(parent)
{
    m_ui.setupUi(this);
    m_ui.defaultProtocol->addItems(protocols);
}

QString SettingsWidget::searchKeywords() const
{
    QString rc;
    QTextStream(&rc) << m_ui.protocolLabel->text() << ' '
            << m_ui.userNameLabel->text();
    rc.remove(QLatin1Char('&'));
    return rc;
}

void SettingsWidget::setSettings(const Settings &settings)
{
    m_ui.userEdit->setText(settings.username);
    const int index = m_ui.defaultProtocol->findText(settings.protocol);
    m_ui.defaultProtocol->setCurrentIndex(index == -1 ? 0  : index);
    m_ui.clipboardBox->setChecked(settings.copyToClipboard);
    m_ui.displayBox->setChecked(settings.displayOutput);
}

Settings SettingsWidget::settings()
{
    Settings rc;
    rc.username = m_ui.userEdit->text();
    rc.protocol = m_ui.defaultProtocol->currentText();
    rc.copyToClipboard = m_ui.clipboardBox->isChecked();
    rc.displayOutput = m_ui.displayBox->isChecked();
    return rc;
}

SettingsPage::SettingsPage(const QSharedPointer<Settings> &settings) :
    m_settings(settings)
{
}

SettingsPage::~SettingsPage()
{
}

QString SettingsPage::id() const
{
    return QLatin1String("A.General");
}

QString SettingsPage::displayName() const
{
    return tr("General");
}

QString SettingsPage::category() const
{
    return QLatin1String(Constants::CPASTER_SETTINGS_CATEGORY);
}

QString SettingsPage::displayCategory() const
{
    return QCoreApplication::translate("CodePaster", Constants::CPASTER_SETTINGS_TR_CATEGORY);
}

QWidget *SettingsPage::createPage(QWidget *parent)
{
    m_widget = new SettingsWidget(m_protocols, parent);
    m_widget->setSettings(*m_settings);

    if (m_searchKeywords.isEmpty())
        m_searchKeywords = m_widget->searchKeywords();
    return m_widget;
}

void SettingsPage::apply()
{
    QTC_ASSERT(m_widget, return)
    const Settings newSettings = m_widget->settings();
    if (newSettings != *m_settings) {
        *m_settings = newSettings;
        m_settings->toSettings(Core::ICore::instance()->settings());
    }
}

bool SettingsPage::matches(const QString &s) const
{
    return m_searchKeywords.contains(s, Qt::CaseInsensitive);
}

void SettingsPage::addProtocol(const QString &name)
{
    m_protocols.append(name);
}

} // namespace CodePaster
