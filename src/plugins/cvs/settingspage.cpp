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
#include "cvssettings.h"
#include "cvsplugin.h"

#include <coreplugin/icore.h>
#include <extensionsystem/pluginmanager.h>
#include <vcsbase/vcsbaseconstants.h>
#include <utils/pathchooser.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QTextStream>
#include <QtGui/QFileDialog>

using namespace CVS::Internal;
using namespace Utils;

SettingsPageWidget::SettingsPageWidget(QWidget *parent) :
    QWidget(parent)
{
    m_ui.setupUi(this);
    m_ui.commandPathChooser->setExpectedKind(PathChooser::Command);
    m_ui.commandPathChooser->setPromptDialogTitle(tr("CVS Command"));
}

CVSSettings SettingsPageWidget::settings() const
{
    CVSSettings rc;
    rc.cvsCommand = m_ui.commandPathChooser->path();
    rc.cvsRoot = m_ui.rootLineEdit->text();
    rc.cvsDiffOptions = m_ui.diffOptionsLineEdit->text();
    rc.timeOutS = m_ui.timeOutSpinBox->value();
    rc.promptToSubmit = m_ui.promptToSubmitCheckBox->isChecked();
    rc.describeByCommitId = m_ui.describeByCommitIdCheckBox->isChecked();
    return rc;
}

void SettingsPageWidget::setSettings(const CVSSettings &s)
{
    m_ui.commandPathChooser->setPath(s.cvsCommand);
    m_ui.rootLineEdit->setText(s.cvsRoot);
    m_ui.diffOptionsLineEdit->setText(s.cvsDiffOptions);
    m_ui.timeOutSpinBox->setValue(s.timeOutS);
    m_ui.promptToSubmitCheckBox->setChecked(s.promptToSubmit);
    m_ui.describeByCommitIdCheckBox->setChecked(s.describeByCommitId);
}

QString SettingsPageWidget::searchKeywords() const
{
    QString rc;
    QTextStream(&rc) << m_ui.promptToSubmitCheckBox->text()
            << ' ' <<  m_ui.describeByCommitIdCheckBox->text()
            << ' ' << m_ui.commandLabel->text()
            << ' ' << m_ui.rootLabel->text() << ' ' << m_ui.diffOptionsLabel->text();
    rc.remove(QLatin1Char('&'));
    return rc;
}

SettingsPage::SettingsPage()
{
}

QString SettingsPage::id() const
{
    return QLatin1String(VCSBase::Constants::VCS_ID_CVS);
}

QString SettingsPage::displayName() const
{
    return tr("CVS");
}

QString SettingsPage::category() const
{
    return QLatin1String(VCSBase::Constants::VCS_SETTINGS_CATEGORY);
}

QString SettingsPage::displayCategory() const
{
    return QCoreApplication::translate("VCSBase", VCSBase::Constants::VCS_SETTINGS_TR_CATEGORY);
}

QWidget *SettingsPage::createPage(QWidget *parent)
{
    m_widget = new SettingsPageWidget(parent);
    m_widget->setSettings(CVSPlugin::cvsPluginInstance()->settings());
    if (m_searchKeywords.isEmpty())
        m_searchKeywords = m_widget->searchKeywords();
    return m_widget;
}

void SettingsPage::apply()
{
    CVSPlugin::cvsPluginInstance()->setSettings(m_widget->settings());
}

bool SettingsPage::matches(const QString &s) const
{
    return m_searchKeywords.contains(s, Qt::CaseInsensitive);
}
