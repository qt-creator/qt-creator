/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
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
    m_ui.commandPathChooser->setExpectedKind(PathChooser::ExistingCommand);
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
    QLatin1Char sep(' ');
    QTextStream(&rc)
            << sep << m_ui.configGroupBox->title()
            << sep << m_ui.commandLabel->text()
            << sep << m_ui.rootLabel->text()
            << sep << m_ui.miscGroupBox->title()
            << sep << m_ui.timeOutLabel->text()
            << sep << m_ui.diffOptionsLabel->text()
            << sep << m_ui.promptToSubmitCheckBox->text()
            << sep << m_ui.describeByCommitIdCheckBox->text()
               ;
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

QWidget *SettingsPage::createPage(QWidget *parent)
{
    m_widget = new SettingsPageWidget(parent);
    m_widget->setSettings(CVSPlugin::instance()->settings());
    if (m_searchKeywords.isEmpty())
        m_searchKeywords = m_widget->searchKeywords();
    return m_widget;
}

void SettingsPage::apply()
{
    CVSPlugin::instance()->setSettings(m_widget->settings());
}

bool SettingsPage::matches(const QString &s) const
{
    return m_searchKeywords.contains(s, Qt::CaseInsensitive);
}
