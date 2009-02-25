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
#include "subversionsettings.h"
#include "subversionplugin.h"

#include <coreplugin/icore.h>
#include <extensionsystem/pluginmanager.h>

#include <QtGui/QFileDialog>
#include <utils/pathchooser.h>

using namespace Subversion::Internal;
using namespace Core::Utils;


SettingsPageWidget::SettingsPageWidget(QWidget *parent) :
    QWidget(parent)
{
    m_ui.setupUi(this);
    m_ui.pathChooser->setExpectedKind(PathChooser::Command);
    m_ui.pathChooser->setPromptDialogTitle(tr("Subversion Command"));
}

SubversionSettings SettingsPageWidget::settings() const
{
    SubversionSettings rc;
    rc.svnCommand = m_ui.pathChooser->path();
    rc.useAuthentication = m_ui.userGroupBox->isChecked();
    rc.user =  m_ui.usernameLineEdit->text();
    rc.password = m_ui.passwordLineEdit->text();
    if (rc.user.isEmpty())
        rc.useAuthentication = false;
    return rc;
}

void SettingsPageWidget::setSettings(const SubversionSettings &s)
{
    m_ui.pathChooser->setPath(s.svnCommand);
    m_ui.usernameLineEdit->setText(s.user);
    m_ui.passwordLineEdit->setText(s.password);
    m_ui.userGroupBox->setChecked(s.useAuthentication);
}

SettingsPage::SettingsPage()
{
}

QString SettingsPage::name() const
{
    return tr("General");
}

QString SettingsPage::category() const
{
    return QLatin1String("Subversion");
}

QString SettingsPage::trCategory() const
{
    return tr("Subversion");
}

QWidget *SettingsPage::createPage(QWidget *parent)
{
    if (!m_widget)
        m_widget = new SettingsPageWidget(parent);
    m_widget->setSettings(SubversionPlugin::subversionPluginInstance()->settings());
    return m_widget;
}

void SettingsPage::apply()
{
    if (!m_widget)
        return;
    SubversionPlugin::subversionPluginInstance()->setSettings(m_widget->settings());
}
