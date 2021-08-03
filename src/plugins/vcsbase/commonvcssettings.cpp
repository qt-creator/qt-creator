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

#include "commonvcssettings.h"

#include "vcsbaseconstants.h"

#include <coreplugin/icore.h>
#include <coreplugin/iversioncontrol.h>
#include <coreplugin/vcsmanager.h>

#include <utils/environment.h>
#include <utils/hostosinfo.h>
#include <utils/layoutbuilder.h>

#include <QDebug>
#include <QPushButton>

using namespace Utils;

namespace VcsBase {
namespace Internal {

// Return default for the ssh-askpass command (default to environment)
static QString sshPasswordPromptDefault()
{
    const QByteArray envSetting = qgetenv("SSH_ASKPASS");
    if (!envSetting.isEmpty())
        return QString::fromLocal8Bit(envSetting);
    if (HostOsInfo::isWindowsHost())
        return QLatin1String("win-ssh-askpass");
    return QLatin1String("ssh-askpass");
}

CommonVcsSettings::CommonVcsSettings()
{
    setSettingsGroup("VCS");
    setAutoApply(false);

    registerAspect(&nickNameMailMap);
    nickNameMailMap.setSettingsKey("NickNameMailMap");
    nickNameMailMap.setDisplayStyle(StringAspect::PathChooserDisplay);
    nickNameMailMap.setExpectedKind(PathChooser::File);
    nickNameMailMap.setHistoryCompleter("Vcs.NickMap.History");
    nickNameMailMap.setLabelText(tr("User/&alias configuration file:"));
    nickNameMailMap.setToolTip(tr("A file listing nicknames in a 4-column mailmap format:\n"
        "'name <email> alias <email>'."));

    registerAspect(&nickNameFieldListFile);
    nickNameFieldListFile.setSettingsKey("NickNameFieldListFile");
    nickNameFieldListFile.setDisplayStyle(StringAspect::PathChooserDisplay);
    nickNameFieldListFile.setExpectedKind(PathChooser::File);
    nickNameFieldListFile.setHistoryCompleter("Vcs.NickFields.History");
    nickNameFieldListFile.setLabelText(tr("User &fields configuration file:"));
    nickNameFieldListFile.setToolTip(tr("A simple file containing lines with field names like "
        "\"Reviewed-By:\" which will be added below the submit editor."));

    registerAspect(&submitMessageCheckScript);
    submitMessageCheckScript.setSettingsKey("SubmitMessageCheckScript");
    submitMessageCheckScript.setDisplayStyle(StringAspect::PathChooserDisplay);
    submitMessageCheckScript.setExpectedKind(PathChooser::ExistingCommand);
    submitMessageCheckScript.setHistoryCompleter("Vcs.MessageCheckScript.History");
    submitMessageCheckScript.setLabelText(tr("Submit message &check script:"));
    submitMessageCheckScript.setToolTip(tr("An executable which is called with the submit message "
        "in a temporary file as first argument. It should return with an exit != 0 and a message "
        "on standard error to indicate failure."));

    registerAspect(&sshPasswordPrompt);
    sshPasswordPrompt.setSettingsKey("SshPasswordPrompt");
    sshPasswordPrompt.setDisplayStyle(StringAspect::PathChooserDisplay);
    sshPasswordPrompt.setExpectedKind(PathChooser::ExistingCommand);
    sshPasswordPrompt.setHistoryCompleter("Vcs.SshPrompt.History");
    sshPasswordPrompt.setDefaultValue(sshPasswordPromptDefault());
    sshPasswordPrompt.setLabelText(tr("&SSH prompt command:"));
    sshPasswordPrompt.setToolTip(tr("Specifies a command that is executed to graphically prompt "
        "for a password,\nshould a repository require SSH-authentication "
        "(see documentation on SSH and the environment variable SSH_ASKPASS)."));

    registerAspect(&lineWrap);
    lineWrap.setSettingsKey("LineWrap");
    lineWrap.setDefaultValue(true);
    lineWrap.setLabelText(tr("Wrap submit message at:"));

    registerAspect(&lineWrapWidth);
    lineWrapWidth.setSettingsKey("LineWrapWidth");
    lineWrapWidth.setSuffix(tr(" characters"));
    lineWrapWidth.setDefaultValue(72);
}

// CommonSettingsWidget

class CommonSettingsWidget final : public Core::IOptionsPageWidget
{
public:
    CommonSettingsWidget(CommonOptionsPage *page);

    void apply() final;

private:
    void updatePath();
    CommonOptionsPage *m_page;
};


CommonSettingsWidget::CommonSettingsWidget(CommonOptionsPage *page)
    : m_page(page)
{
    CommonVcsSettings &s = m_page->settings();

    auto cacheResetButton = new QPushButton(CommonVcsSettings::tr("Reset VCS Cache"));
    cacheResetButton->setToolTip(CommonVcsSettings::tr("Reset information about which "
        "version control system handles which directory."));

    updatePath();

    using namespace Layouting;
    Column {
        Row { s.lineWrap, s.lineWrapWidth, Stretch() },
        Form {
            s.submitMessageCheckScript,
            s.nickNameMailMap,
            s.nickNameFieldListFile,
            s.sshPasswordPrompt,
            {}, cacheResetButton
        }
    }.attachTo(this);

    connect(Core::VcsManager::instance(), &Core::VcsManager::configurationChanged,
            this, &CommonSettingsWidget::updatePath);
    connect(cacheResetButton, &QPushButton::clicked,
            Core::VcsManager::instance(), &Core::VcsManager::clearVersionControlCache);
}

void CommonSettingsWidget::updatePath()
{
    EnvironmentChange change;
    change.addAppendToPath(Core::VcsManager::additionalToolsPath());
    m_page->settings().sshPasswordPrompt.setEnvironmentChange(change);
}

void CommonSettingsWidget::apply()
{
    CommonVcsSettings &s = m_page->settings();
    if (s.isDirty()) {
        s.apply();
        s.writeSettings(Core::ICore::settings());
        emit m_page->settingsChanged();
    }
}

// CommonOptionsPage

CommonOptionsPage::CommonOptionsPage()
{
    m_settings.readSettings(Core::ICore::settings());

    setId(Constants::VCS_COMMON_SETTINGS_ID);
    setDisplayName(QCoreApplication::translate("VcsBase", Constants::VCS_COMMON_SETTINGS_NAME));
    setCategory(Constants::VCS_SETTINGS_CATEGORY);
    // The following act as blueprint for other pages in the same category:
    setDisplayCategory(QCoreApplication::translate("VcsBase", "Version Control"));
    setCategoryIconPath(":/vcsbase/images/settingscategory_vcs.png");
    setWidgetCreator([this] { return new CommonSettingsWidget(this); });
}

} // namespace Internal
} // namespace VcsBase
