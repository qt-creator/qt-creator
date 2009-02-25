/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "settingspage.h"

#include <coreplugin/icore.h>

#include <QtCore/QSettings>
#include <QtGui/QLineEdit>
#include <QtGui/QFileDialog>
#include <QtCore/QDebug>
#include <QtCore/QVariant>

using namespace CodePaster;

SettingsPage::SettingsPage()
{
    m_settings = Core::ICore::instance()->settings();
    if (m_settings) {
        m_settings->beginGroup("CodePaster");
        m_username = m_settings->value("UserName", qgetenv("USER")).toString();
        m_server = m_settings->value("Server", "<no url>").toUrl();
        m_copy = m_settings->value("CopyToClipboard", true).toBool();
        m_output = m_settings->value("DisplayOutput", true).toBool();
        m_settings->endGroup();
    }
}

QString SettingsPage::name() const
{
    return "General";
}

QString SettingsPage::category() const
{
    return "CodePaster";
}

QString SettingsPage::trCategory() const
{
    return tr("CodePaster");
}

QWidget *SettingsPage::createPage(QWidget *parent)
{
    QWidget *w = new QWidget(parent);
    m_ui.setupUi(w);
    m_ui.userEdit->setText(m_username);
    m_ui.serverEdit->setText(m_server.toString());
    m_ui.clipboardBox->setChecked(m_copy);
    m_ui.displayBox->setChecked(m_output);
    return w;
}

void SettingsPage::apply()
{
    m_username = m_ui.userEdit->text();
    m_server = QUrl(m_ui.serverEdit->text());
    m_copy = m_ui.clipboardBox->isChecked();
    m_output = m_ui.displayBox->isChecked();

    if (!m_settings)
        return;

    m_settings->beginGroup("CodePaster");
    m_settings->setValue("UserName", m_username);
    m_settings->setValue("Server", m_server);
    m_settings->setValue("CopyToClipboard", m_copy);
    m_settings->setValue("DisplayOutput", m_output);
    m_settings->endGroup();
}

QString SettingsPage::username() const
{
    return m_username;
}

QUrl SettingsPage::serverUrl() const
{
    return m_server;
}
