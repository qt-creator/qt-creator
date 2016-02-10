/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "settingspage.h"
#include "perforcesettings.h"
#include "perforceplugin.h"
#include "perforcechecker.h"

#include <vcsbase/vcsbaseconstants.h>

#include <QApplication>
#include <QLineEdit>
#include <QFileDialog>
#include <QTextStream>

using namespace Perforce::Internal;
using namespace Utils;

SettingsPageWidget::SettingsPageWidget(QWidget *parent) :
    QWidget(parent)
{
    m_ui.setupUi(this);
    m_ui.errorLabel->clear();
    m_ui.pathChooser->setPromptDialogTitle(tr("Perforce Command"));
    m_ui.pathChooser->setHistoryCompleter(QLatin1String("Perforce.Command.History"));
    m_ui.pathChooser->setExpectedKind(PathChooser::Command);
    connect(m_ui.testPushButton, &QPushButton::clicked, this, &SettingsPageWidget::slotTest);
}

SettingsPageWidget::~SettingsPageWidget()
{
    delete m_checker;
}

void SettingsPageWidget::slotTest()
{
    if (!m_checker) {
        m_checker = new PerforceChecker(this);
        m_checker->setUseOverideCursor(true);
        connect(m_checker, &PerforceChecker::failed, this, &SettingsPageWidget::setStatusError);
        connect(m_checker, &PerforceChecker::succeeded, this, &SettingsPageWidget::testSucceeded);
    }

    if (m_checker->isRunning())
        return;

    setStatusText(tr("Testing..."));
    const Settings s = settings();
    m_checker->start(s.p4BinaryPath, QString(), s.commonP4Arguments(), 10000);
}

void SettingsPageWidget::testSucceeded(const QString &repo)
{
    setStatusText(tr("Test succeeded (%1).").arg(QDir::toNativeSeparators(repo)));
}

Settings SettingsPageWidget::settings() const
{
    Settings  settings;
    settings.p4Command = m_ui.pathChooser->rawPath();
    settings.p4BinaryPath = m_ui.pathChooser->path();
    settings.defaultEnv = !m_ui.environmentGroupBox->isChecked();
    settings.p4Port = m_ui.portLineEdit->text();
    settings.p4User = m_ui.userLineEdit->text();
    settings.p4Client=  m_ui.clientLineEdit->text();
    settings.timeOutS = m_ui.timeOutSpinBox->value();
    settings.logCount = m_ui.logCountSpinBox->value();
    settings.promptToSubmit = m_ui.promptToSubmitCheckBox->isChecked();
    settings.autoOpen = m_ui.autoOpenCheckBox->isChecked();
    return settings;
}

void SettingsPageWidget::setSettings(const PerforceSettings &s)
{
    m_ui.pathChooser->setPath(s.p4Command());
    m_ui.environmentGroupBox->setChecked(!s.defaultEnv());
    m_ui.portLineEdit->setText(s.p4Port());
    m_ui.clientLineEdit->setText(s.p4Client());
    m_ui.userLineEdit->setText(s.p4User());
    m_ui.logCountSpinBox->setValue(s.logCount());
    m_ui.timeOutSpinBox->setValue(s.timeOutS());
    m_ui.promptToSubmitCheckBox->setChecked(s.promptToSubmit());
    m_ui.autoOpenCheckBox->setChecked(s.autoOpen());
}

void SettingsPageWidget::setStatusText(const QString &t)
{
    m_ui.errorLabel->setStyleSheet(QString());
    m_ui.errorLabel->setText(t);
}

void SettingsPageWidget::setStatusError(const QString &t)
{
    m_ui.errorLabel->setStyleSheet(QLatin1String("background-color: red"));
    m_ui.errorLabel->setText(t);
}

SettingsPage::SettingsPage()
{
    setId(VcsBase::Constants::VCS_ID_PERFORCE);
    setDisplayName(tr("Perforce"));
}

SettingsPage::~SettingsPage()
{
    delete m_widget;
}

QWidget *SettingsPage::widget()
{
    if (!m_widget) {
        m_widget = new SettingsPageWidget;
        m_widget->setSettings(PerforcePlugin::settings());
    }
    return m_widget;
}

void SettingsPage::apply()
{
    PerforcePlugin::setSettings(m_widget->settings());
}

void SettingsPage::finish()
{
    delete m_widget;
    m_widget = nullptr;
}
