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
#include "subversionsettings.h"
#include "subversionplugin.h"

#include <coreplugin/icore.h>
#include <extensionsystem/pluginmanager.h>
#include <vcsbase/vcsbaseconstants.h>
#include <utils/pathchooser.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QTextStream>
#include <QtGui/QFileDialog>

using namespace Subversion::Internal;
using namespace Utils;

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
    rc.timeOutS = m_ui.timeOutSpinBox->value();
    if (rc.user.isEmpty())
        rc.useAuthentication = false;
    rc.promptToSubmit = m_ui.promptToSubmitCheckBox->isChecked();
    return rc;
}

void SettingsPageWidget::setSettings(const SubversionSettings &s)
{
    m_ui.pathChooser->setPath(s.svnCommand);
    m_ui.usernameLineEdit->setText(s.user);
    m_ui.passwordLineEdit->setText(s.password);
    m_ui.userGroupBox->setChecked(s.useAuthentication);
    m_ui.timeOutSpinBox->setValue(s.timeOutS);
    m_ui.promptToSubmitCheckBox->setChecked(s.promptToSubmit);
}

QString SettingsPageWidget::searchKeywords() const
{
    QString rc;
    QTextStream(&rc) << m_ui.commandLabel->text()
            << ' ' << m_ui.usernameLabel->text()
            << ' ' << m_ui.passwordLabel->text()
            << ' ' << m_ui.userGroupBox->title();
    rc.remove(QLatin1Char('&'));
    return rc;
}

SettingsPage::SettingsPage() :
    m_widget(0)
{
}

QString SettingsPage::id() const
{
    return QLatin1String(VCSBase::Constants::VCS_ID_SUBVERSION);
}

QString SettingsPage::trName() const
{
    return tr("Subversion");
}

QString SettingsPage::category() const
{
    return QLatin1String(VCSBase::Constants::VCS_SETTINGS_CATEGORY);
}

QString SettingsPage::trCategory() const
{
    return QCoreApplication::translate("VCSBase", VCSBase::Constants::VCS_SETTINGS_TR_CATEGORY);
}

QWidget *SettingsPage::createPage(QWidget *parent)
{
    m_widget = new SettingsPageWidget(parent);
    m_widget->setSettings(SubversionPlugin::subversionPluginInstance()->settings());
    if (m_searchKeywords.isEmpty())
        m_searchKeywords = m_widget->searchKeywords();
    return m_widget;
}

void SettingsPage::apply()
{
    SubversionPlugin::subversionPluginInstance()->setSettings(m_widget->settings());
}

bool SettingsPage::matches(const QString &s) const
{
    return m_searchKeywords.contains(s, Qt::CaseInsensitive);
}
