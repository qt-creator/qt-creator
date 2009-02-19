/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

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
