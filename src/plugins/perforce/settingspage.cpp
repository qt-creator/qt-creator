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
#include "perforcesettings.h"
#include "perforceplugin.h"

#include <QtGui/QLineEdit>
#include <QtGui/QFileDialog>

using namespace Perforce::Internal;
using namespace Core::Utils;

SettingsPageWidget::SettingsPageWidget(QWidget *parent) :
    QWidget(parent)
{
    m_ui.setupUi(this);
    m_ui.pathChooser->setPromptDialogTitle(tr("Perforce Command"));
    m_ui.pathChooser->setExpectedKind(PathChooser::Command);
}

QString SettingsPageWidget::p4Command() const
{
    return m_ui.pathChooser->path();
}

bool SettingsPageWidget::defaultEnv() const
{
    return m_ui.defaultCheckBox->isChecked();
}

QString SettingsPageWidget::p4Port() const
{
    return m_ui.portLineEdit->text();
}

QString SettingsPageWidget::p4User() const
{
    return m_ui.userLineEdit->text();
}

QString SettingsPageWidget::p4Client() const
{
    return m_ui.clientLineEdit->text();
}

void SettingsPageWidget::setSettings(const PerforceSettings &s)
{
    m_ui.pathChooser->setPath(s.p4Command());
    m_ui.defaultCheckBox->setChecked(s.defaultEnv());
    m_ui.portLineEdit->setText(s.p4Port());
    m_ui.clientLineEdit->setText(s.p4Client());
    m_ui.userLineEdit->setText(s.p4User());
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
    return QLatin1String("Perforce");
}

QString SettingsPage::trCategory() const
{
    return tr("Perforce");
}

QWidget *SettingsPage::createPage(QWidget *parent)
{
    if (!m_widget)
        m_widget = new SettingsPageWidget(parent);
    m_widget->setSettings(PerforcePlugin::perforcePluginInstance()->settings());
    return m_widget;
}

void SettingsPage::apply()
{
    if (!m_widget)
        return;

    PerforcePlugin::perforcePluginInstance()->setSettings(m_widget->p4Command(), m_widget->p4Port(), m_widget->p4Client(), m_widget->p4User(), m_widget->defaultEnv());
}
