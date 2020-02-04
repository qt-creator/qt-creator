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

using namespace Utils;

namespace Perforce {
namespace Internal {

class SettingsPageWidget final : public Core::IOptionsPageWidget
{
    Q_DECLARE_TR_FUNCTIONS(Perforce::Internal::SettingsPage)

public:
    SettingsPageWidget(PerforceSettings *settings, const std::function<void()> &onApply);
    ~SettingsPageWidget() final;

private:
    void apply() final;

    Settings settings() const;

    void slotTest();
    void setStatusText(const QString &);
    void setStatusError(const QString &);
    void testSucceeded(const QString &repo);

    Ui::SettingsPage m_ui;
    PerforceChecker *m_checker = nullptr;
    PerforceSettings *m_settings = nullptr;
    std::function<void()> m_onApply;
};

SettingsPageWidget::SettingsPageWidget(PerforceSettings *settings, const std::function<void()> &onApply)
    : m_settings(settings), m_onApply(onApply)
{
    m_ui.setupUi(this);
    m_ui.errorLabel->clear();
    m_ui.pathChooser->setPromptDialogTitle(tr("Perforce Command"));
    m_ui.pathChooser->setHistoryCompleter(QLatin1String("Perforce.Command.History"));
    m_ui.pathChooser->setExpectedKind(PathChooser::Command);
    connect(m_ui.testPushButton, &QPushButton::clicked, this, &SettingsPageWidget::slotTest);

    const PerforceSettings &s = *settings;
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
    const Settings s = m_settings->settings();
    m_checker->start(s.p4BinaryPath, QString(), s.commonP4Arguments(), 10000);
}

void SettingsPageWidget::testSucceeded(const QString &repo)
{
    setStatusText(tr("Test succeeded (%1).").arg(QDir::toNativeSeparators(repo)));
}

void SettingsPageWidget::apply()
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

    if (settings == m_settings->settings())
        return;

    m_settings->setSettings(settings);
    m_onApply();
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

SettingsPage::SettingsPage(PerforceSettings *settings, const std::function<void ()> &onApply)
{
    setId(VcsBase::Constants::VCS_ID_PERFORCE);
    setDisplayName(SettingsPageWidget::tr("Perforce"));
    setCategory(VcsBase::Constants::VCS_SETTINGS_CATEGORY);
    setWidgetCreator([settings, onApply] { return new SettingsPageWidget(settings, onApply); });
}

} // Internal
} // Perforce
