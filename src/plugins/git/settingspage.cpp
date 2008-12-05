/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
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
** version 1.2, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#include "settingspage.h"
#include "gitsettings.h"
#include "gitplugin.h"

#include <QtCore/QDebug>

using namespace Git::Internal;

SettingsPageWidget::SettingsPageWidget(QWidget *parent) :
    QWidget(parent)
{
    m_ui.setupUi(this);
    connect(m_ui.adoptButton, SIGNAL(clicked()), this, SLOT(setSystemPath()));
}

GitSettings SettingsPageWidget::settings() const
{
    GitSettings rc;
    rc.path = m_ui.pathLineEdit->text();
    rc.adoptPath = m_ui.environmentGroupBox->isChecked() && !rc.path.isEmpty();
    rc.logCount = m_ui.logCountSpinBox->value();
    return rc;
}

void SettingsPageWidget::setSettings(const GitSettings &s)
{
    m_ui.environmentGroupBox->setChecked(s.adoptPath);
    m_ui.pathLineEdit->setText(s.path);
    m_ui.logCountSpinBox->setValue(s.logCount);
}

void SettingsPageWidget::setSystemPath()
{
    m_ui.pathLineEdit->setText(QLatin1String(qgetenv("PATH")));
}

// -------- SettingsPage
SettingsPage::SettingsPage()
{
}

QString SettingsPage::name() const
{
    return tr("General");
}

 QString SettingsPage::category() const
{
    return QLatin1String("Git");
}

QString SettingsPage::trCategory() const
{
    return tr("Git");
}

QWidget *SettingsPage::createPage(QWidget *parent)
{
    if (!m_widget)
        m_widget = new SettingsPageWidget(parent);
    m_widget->setSettings(GitPlugin::instance()->settings());
    return m_widget;
}

void SettingsPage::finished(bool accepted)
{
    if (!accepted || !m_widget)
        return;

    GitPlugin::instance()->setSettings(m_widget->settings());
}

