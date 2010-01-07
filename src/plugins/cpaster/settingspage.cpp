/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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
#include "cpasterconstants.h"

#include <coreplugin/icore.h>

#include <QtGui/QLineEdit>
#include <QtGui/QFileDialog>

#include <QtCore/QDebug>
#include <QtCore/QVariant>
#include <QtCore/QSettings>
#include <QtCore/QTextStream>
#include <QtCore/QCoreApplication>

using namespace CodePaster;

SettingsPage::SettingsPage()
{
    m_settings = Core::ICore::instance()->settings();
    if (m_settings) {
        m_settings->beginGroup("CodePaster");
#ifdef Q_OS_WIN32
        QString defaultUser = qgetenv("USERNAME");
#else
        QString defaultUser = qgetenv("USER");
#endif
        m_username = m_settings->value("UserName", defaultUser).toString();
        m_protocol = m_settings->value("DefaultProtocol", "CodePaster").toString();
        m_copy = m_settings->value("CopyToClipboard", true).toBool();
        m_output = m_settings->value("DisplayOutput", true).toBool();
        m_settings->endGroup();
    }
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
    QWidget *w = new QWidget(parent);
    m_ui.setupUi(w);
    m_ui.defaultProtocol->clear();
    m_ui.defaultProtocol->insertItems(0, m_protocols);
    m_ui.userEdit->setText(m_username);
    m_ui.clipboardBox->setChecked(m_copy);
    m_ui.displayBox->setChecked(m_output);
    if (m_searchKeywords.isEmpty()) {
        QTextStream(&m_searchKeywords) << m_ui.protocolLabel->text() << ' '
                << m_ui.userNameLabel->text();
        m_searchKeywords.remove(QLatin1Char('&'));
    }
    return w;
}

void SettingsPage::apply()
{
    m_username = m_ui.userEdit->text();
    m_protocol = m_ui.defaultProtocol->currentText();
    m_copy = m_ui.clipboardBox->isChecked();
    m_output = m_ui.displayBox->isChecked();

    if (!m_settings)
        return;

    m_settings->beginGroup("CodePaster");
    m_settings->setValue("UserName", m_username);
    m_settings->setValue("DefaultProtocol", m_protocol);
    m_settings->setValue("CopyToClipboard", m_copy);
    m_settings->setValue("DisplayOutput", m_output);
    m_settings->endGroup();
}

bool SettingsPage::matches(const QString &s) const
{
    return m_searchKeywords.contains(s, Qt::CaseInsensitive);
}

void SettingsPage::addProtocol(const QString &name)
{
    m_protocols.append(name);
}

QString SettingsPage::username() const
{
    return m_username;
}

QString SettingsPage::defaultProtocol() const
{
    return m_protocol;
}
