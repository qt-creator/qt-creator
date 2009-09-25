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
#include "gitsettings.h"
#include "gitplugin.h"

#include <vcsbase/vcsbaseconstants.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QFileInfo>
#include <QtCore/QDebug>
#include <QtGui/QMessageBox>

namespace Git {
namespace Internal {

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
    rc.timeout = m_ui.timeoutSpinBox->value();
    rc.promptToSubmit = m_ui.promptToSubmitCheckBox->isChecked();
    rc.omitAnnotationDate = m_ui.omitAnnotationDataCheckBox->isChecked();
    return rc;
}

void SettingsPageWidget::setSettings(const GitSettings &s)
{
    m_ui.environmentGroupBox->setChecked(s.adoptPath);
    m_ui.pathLineEdit->setText(s.path);
    m_ui.logCountSpinBox->setValue(s.logCount);
    m_ui.timeoutSpinBox->setValue(s.timeout);
    m_ui.promptToSubmitCheckBox->setChecked(s.promptToSubmit);
    m_ui.omitAnnotationDataCheckBox->setChecked(s.omitAnnotationDate);
}

void SettingsPageWidget::setSystemPath()
{
    m_ui.pathLineEdit->setText(QLatin1String(qgetenv("PATH")));
}

// -------- SettingsPage
SettingsPage::SettingsPage()
{
}

QString SettingsPage::id() const
{
    return QLatin1String("Git");
}

QString SettingsPage::trName() const
{
    return tr("Git");
}

QString SettingsPage::category() const
{
    return QLatin1String(VCSBase::Constants::VCS_SETTINGS_CATEGORY);
}

QString SettingsPage::trCategory() const
{
    return QCoreApplication::translate("VCSBase", VCSBase::Constants::VCS_SETTINGS_CATEGORY);
}

QWidget *SettingsPage::createPage(QWidget *parent)
{
    m_widget = new SettingsPageWidget(parent);
    m_widget->setSettings(GitPlugin::instance()->settings());
    return m_widget;
}

void SettingsPage::apply()
{
    const GitSettings newSettings = m_widget->settings();
    // Warn if git cannot be found in path if the widget is on top
    if (m_widget->isVisible()) {
        bool gitFoundOk;
        QString errorMessage;
        newSettings.gitBinaryPath(&gitFoundOk, &errorMessage);
        if (!gitFoundOk)
            QMessageBox::warning(m_widget, tr("Git Settings"), errorMessage);
    }

    GitPlugin::instance()->setSettings(newSettings);
}
}
}
