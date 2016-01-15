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

#include "commonsettingspage.h"
#include "vcsbaseconstants.h"

#include "ui_commonsettingspage.h"

#include <coreplugin/icore.h>
#include <coreplugin/iversioncontrol.h>
#include <coreplugin/vcsmanager.h>

#include <utils/environment.h>
#include <utils/hostosinfo.h>

#include <QCoreApplication>

namespace VcsBase {
namespace Internal {

// ------------------ VcsBaseSettingsWidget

CommonSettingsWidget::CommonSettingsWidget(QWidget *parent) :
    QWidget(parent),
    m_ui(new Ui::CommonSettingsPage)
{
    m_ui->setupUi(this);
    m_ui->submitMessageCheckScriptChooser->setExpectedKind(Utils::PathChooser::ExistingCommand);
    m_ui->submitMessageCheckScriptChooser->setHistoryCompleter(QLatin1String("Vcs.MessageCheckScript.History"));
    m_ui->nickNameFieldsFileChooser->setExpectedKind(Utils::PathChooser::File);
    m_ui->nickNameFieldsFileChooser->setHistoryCompleter(QLatin1String("Vcs.NickFields.History"));
    m_ui->nickNameMailMapChooser->setExpectedKind(Utils::PathChooser::File);
    m_ui->nickNameMailMapChooser->setHistoryCompleter(QLatin1String("Vcs.NickMap.History"));
    m_ui->sshPromptChooser->setExpectedKind(Utils::PathChooser::ExistingCommand);
    m_ui->sshPromptChooser->setHistoryCompleter(QLatin1String("Vcs.SshPrompt.History"));

    updatePath();

    connect(Core::VcsManager::instance(), &Core::VcsManager::configurationChanged,
            this, &CommonSettingsWidget::updatePath);
    connect(m_ui->cacheResetButton, &QPushButton::clicked,
            this, [] { Core::VcsManager::clearVersionControlCache(); });
}

CommonSettingsWidget::~CommonSettingsWidget()
{
    delete m_ui;
}

CommonVcsSettings CommonSettingsWidget::settings() const
{
    CommonVcsSettings rc;
    rc.nickNameMailMap = m_ui->nickNameMailMapChooser->path();
    rc.nickNameFieldListFile = m_ui->nickNameFieldsFileChooser->path();
    rc.submitMessageCheckScript = m_ui->submitMessageCheckScriptChooser->path();
    rc.lineWrap= m_ui->lineWrapCheckBox->isChecked();
    rc.lineWrapWidth = m_ui->lineWrapSpinBox->value();
    rc.sshPasswordPrompt = m_ui->sshPromptChooser->path();
    return rc;
}

void CommonSettingsWidget::setSettings(const CommonVcsSettings &s)
{
    m_ui->nickNameMailMapChooser->setPath(s.nickNameMailMap);
    m_ui->nickNameFieldsFileChooser->setPath(s.nickNameFieldListFile);
    m_ui->submitMessageCheckScriptChooser->setPath(s.submitMessageCheckScript);
    m_ui->lineWrapCheckBox->setChecked(s.lineWrap);
    m_ui->lineWrapSpinBox->setValue(s.lineWrapWidth);
    m_ui->sshPromptChooser->setPath(s.sshPasswordPrompt);
}

void CommonSettingsWidget::updatePath()
{
    Utils::Environment env = Utils::Environment::systemEnvironment();
    QStringList toAdd = Core::VcsManager::additionalToolsPath();
    env.appendOrSetPath(toAdd.join(Utils::HostOsInfo::pathListSeparator()));
    m_ui->sshPromptChooser->setEnvironment(env);
}

// --------------- VcsBaseSettingsPage
CommonOptionsPage::CommonOptionsPage(QObject *parent) :
    VcsBaseOptionsPage(parent)
{
    m_settings.fromSettings(Core::ICore::settings());

    setId(Constants::VCS_COMMON_SETTINGS_ID);
    setDisplayName(QCoreApplication::translate("VcsBase", Constants::VCS_COMMON_SETTINGS_NAME));
}

QWidget *CommonOptionsPage::widget()
{
    if (!m_widget) {
        m_widget = new CommonSettingsWidget;
        m_widget->setSettings(m_settings);
    }
    return m_widget;
}

void CommonOptionsPage::apply()
{
    if (m_widget) {
        const CommonVcsSettings newSettings = m_widget->settings();
        if (newSettings != m_settings) {
            m_settings = newSettings;
            m_settings.toSettings(Core::ICore::settings());
            emit settingsChanged(m_settings);
        }
    }
}

void CommonOptionsPage::finish()
{
    delete m_widget;
}

} // namespace Internal
} // namespace VcsBase
