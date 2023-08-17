// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "commonvcssettings.h"

#include "vcsbaseconstants.h"
#include "vcsbasetr.h"

#include <coreplugin/dialogs/ioptionspage.h>
#include <coreplugin/iversioncontrol.h>
#include <coreplugin/vcsmanager.h>

#include <utils/environment.h>
#include <utils/hostosinfo.h>
#include <utils/layoutbuilder.h>

using namespace Core;
using namespace Utils;

namespace VcsBase::Internal {

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

CommonVcsSettings &commonSettings()
{
    static CommonVcsSettings settings;
    return settings;
}

CommonVcsSettings::CommonVcsSettings()
{
    setAutoApply(false);
    setSettingsGroup("VCS");

    nickNameMailMap.setSettingsKey("NickNameMailMap");
    nickNameMailMap.setExpectedKind(PathChooser::File);
    nickNameMailMap.setHistoryCompleter("Vcs.NickMap.History");
    nickNameMailMap.setLabelText(Tr::tr("User/&alias configuration file:"));
    nickNameMailMap.setToolTip(Tr::tr("A file listing nicknames in a 4-column mailmap format:\n"
        "'name <email> alias <email>'."));

    nickNameFieldListFile.setSettingsKey("NickNameFieldListFile");
    nickNameFieldListFile.setExpectedKind(PathChooser::File);
    nickNameFieldListFile.setHistoryCompleter("Vcs.NickFields.History");
    nickNameFieldListFile.setLabelText(Tr::tr("User &fields configuration file:"));
    nickNameFieldListFile.setToolTip(Tr::tr("A simple file containing lines with field names like "
        "\"Reviewed-By:\" which will be added below the submit editor."));

    submitMessageCheckScript.setSettingsKey("SubmitMessageCheckScript");
    submitMessageCheckScript.setExpectedKind(PathChooser::ExistingCommand);
    submitMessageCheckScript.setHistoryCompleter("Vcs.MessageCheckScript.History");
    submitMessageCheckScript.setLabelText(Tr::tr("Submit message &check script:"));
    submitMessageCheckScript.setToolTip(Tr::tr("An executable which is called with the submit message "
        "in a temporary file as first argument. It should return with an exit != 0 and a message "
        "on standard error to indicate failure."));

    sshPasswordPrompt.setSettingsKey("SshPasswordPrompt");
    sshPasswordPrompt.setExpectedKind(PathChooser::ExistingCommand);
    sshPasswordPrompt.setHistoryCompleter("Vcs.SshPrompt.History");
    sshPasswordPrompt.setDefaultValue(sshPasswordPromptDefault());
    sshPasswordPrompt.setLabelText(Tr::tr("&SSH prompt command:"));
    sshPasswordPrompt.setToolTip(Tr::tr("Specifies a command that is executed to graphically prompt "
        "for a password,\nshould a repository require SSH-authentication "
        "(see documentation on SSH and the environment variable SSH_ASKPASS)."));

    lineWrap.setSettingsKey("LineWrap");
    lineWrap.setDefaultValue(true);
    lineWrap.setLabelText(Tr::tr("Wrap submit message at:"));

    lineWrapWidth.setSettingsKey("LineWrapWidth");
    lineWrapWidth.setSuffix(Tr::tr(" characters"));
    lineWrapWidth.setDefaultValue(72);

    setLayouter([this] {
        using namespace Layouting;
        return Column {
            Row { lineWrap, lineWrapWidth, st },
            Form {
                submitMessageCheckScript, br,
                nickNameMailMap, br,
                nickNameFieldListFile, br,
                sshPasswordPrompt, br,
                empty,
                PushButton {
                    text(Tr::tr("Reset VCS Cache")),
                    tooltip(Tr::tr("Reset information about which "
                                   "version control system handles which directory.")),
                    onClicked(&VcsManager::clearVersionControlCache)
                }
            }
        };
    });

    auto updatePath = [this] {
        Environment env;
        env.appendToPath(VcsManager::additionalToolsPath());
        sshPasswordPrompt.setEnvironment(env);
    };

    updatePath();
    connect(VcsManager::instance(), &VcsManager::configurationChanged, this, updatePath);

    readSettings();
}

// CommonVcsSettingsPage

class CommonVcsSettingsPage final : public Core::IOptionsPage
{
public:
    CommonVcsSettingsPage()
    {
        setId(Constants::VCS_COMMON_SETTINGS_ID);
        setDisplayName(Tr::tr("General"));
        setCategory(Constants::VCS_SETTINGS_CATEGORY);
        // The following act as blueprint for other pages in the same category:
        setDisplayCategory(Tr::tr("Version Control"));
        setCategoryIconPath(":/vcsbase/images/settingscategory_vcs.png");
        setSettingsProvider([] { return &commonSettings(); });
    }
};

const CommonVcsSettingsPage settingsPage;

} // VcsBase::Internal
