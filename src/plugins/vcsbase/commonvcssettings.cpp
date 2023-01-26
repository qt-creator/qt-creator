// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "commonvcssettings.h"

#include "vcsbaseconstants.h"
#include "vcsbasetr.h"

#include <coreplugin/icore.h>
#include <coreplugin/iversioncontrol.h>
#include <coreplugin/vcsmanager.h>

#include <utils/algorithm.h>
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
    const QString envSetting = qtcEnvironmentVariable("SSH_ASKPASS");
    if (!envSetting.isEmpty())
        return envSetting;
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
    nickNameMailMap.setLabelText(Tr::tr("User/&alias configuration file:"));
    nickNameMailMap.setToolTip(Tr::tr("A file listing nicknames in a 4-column mailmap format:\n"
        "'name <email> alias <email>'."));

    registerAspect(&nickNameFieldListFile);
    nickNameFieldListFile.setSettingsKey("NickNameFieldListFile");
    nickNameFieldListFile.setDisplayStyle(StringAspect::PathChooserDisplay);
    nickNameFieldListFile.setExpectedKind(PathChooser::File);
    nickNameFieldListFile.setHistoryCompleter("Vcs.NickFields.History");
    nickNameFieldListFile.setLabelText(Tr::tr("User &fields configuration file:"));
    nickNameFieldListFile.setToolTip(Tr::tr("A simple file containing lines with field names like "
        "\"Reviewed-By:\" which will be added below the submit editor."));

    registerAspect(&submitMessageCheckScript);
    submitMessageCheckScript.setSettingsKey("SubmitMessageCheckScript");
    submitMessageCheckScript.setDisplayStyle(StringAspect::PathChooserDisplay);
    submitMessageCheckScript.setExpectedKind(PathChooser::ExistingCommand);
    submitMessageCheckScript.setHistoryCompleter("Vcs.MessageCheckScript.History");
    submitMessageCheckScript.setLabelText(Tr::tr("Submit message &check script:"));
    submitMessageCheckScript.setToolTip(Tr::tr("An executable which is called with the submit message "
        "in a temporary file as first argument. It should return with an exit != 0 and a message "
        "on standard error to indicate failure."));

    registerAspect(&sshPasswordPrompt);
    sshPasswordPrompt.setSettingsKey("SshPasswordPrompt");
    sshPasswordPrompt.setDisplayStyle(StringAspect::PathChooserDisplay);
    sshPasswordPrompt.setExpectedKind(PathChooser::ExistingCommand);
    sshPasswordPrompt.setHistoryCompleter("Vcs.SshPrompt.History");
    sshPasswordPrompt.setDefaultValue(sshPasswordPromptDefault());
    sshPasswordPrompt.setLabelText(Tr::tr("&SSH prompt command:"));
    sshPasswordPrompt.setToolTip(Tr::tr("Specifies a command that is executed to graphically prompt "
        "for a password,\nshould a repository require SSH-authentication "
        "(see documentation on SSH and the environment variable SSH_ASKPASS)."));

    registerAspect(&lineWrap);
    lineWrap.setSettingsKey("LineWrap");
    lineWrap.setDefaultValue(true);
    lineWrap.setLabelText(Tr::tr("Wrap submit message at:"));

    registerAspect(&lineWrapWidth);
    lineWrapWidth.setSettingsKey("LineWrapWidth");
    lineWrapWidth.setSuffix(Tr::tr(" characters"));
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

    auto cacheResetButton = new QPushButton(Tr::tr("Reset VCS Cache"));
    cacheResetButton->setToolTip(Tr::tr("Reset information about which "
        "version control system handles which directory."));

    updatePath();

    using namespace Layouting;
    Column {
        Row { s.lineWrap, s.lineWrapWidth, st },
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
    Environment env;
    env.appendToPath(Core::VcsManager::additionalToolsPath());
    m_page->settings().sshPasswordPrompt.setEnvironment(env);
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
    setDisplayName(Tr::tr("General"));
    setCategory(Constants::VCS_SETTINGS_CATEGORY);
    // The following act as blueprint for other pages in the same category:
    setDisplayCategory(Tr::tr("Version Control"));
    setCategoryIconPath(":/vcsbase/images/settingscategory_vcs.png");
    setWidgetCreator([this] { return new CommonSettingsWidget(this); });
}

} // namespace Internal
} // namespace VcsBase
