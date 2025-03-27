// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "extensionmanagersettings.h"

#include "extensionmanagerconstants.h"
#include "extensionmanagertr.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/dialogs/ioptionspage.h>
#include <coreplugin/icore.h>
#include <coreplugin/plugininstallwizard.h>

#include <utils/layoutbuilder.h>
#include <utils/stylehelper.h>

#include <QGuiApplication>
#include <QSslSocket>

namespace ExtensionManager::Internal {

ExtensionManagerSettings &settings()
{
    static ExtensionManagerSettings theExtensionManagerSettings;
    return theExtensionManagerSettings;
}

ExtensionManagerSettings::ExtensionManagerSettings()
{
    setAutoApply(false);
    setSettingsGroup("ExtensionManager");

    useExternalRepo.setSettingsKey("UseExternalRepo");
    useExternalRepo.setDefaultValue(false);
    useExternalRepo.setLabelText(Tr::tr("Use external repository"));

    const bool sslSupported = QSslSocket::supportsSsl();

    useExternalRepo.setEnabled(sslSupported);
    if (!sslSupported) {
        useExternalRepo.setToolTip(Tr::tr("SSL support is not available."));
    }

    repositoryUrls.setSettingsKey("RepositoryUrls");
    repositoryUrls.setLabelText(Tr::tr("Repository Urls:"));
    repositoryUrls.setToolTip(
        Tr::tr("Repositories to query for Extensions. You can specify local paths or "
               "http(s) urls that should be merged with the main repository."));
    repositoryUrls.setDefaultValue(
        {"https://github.com/qt-creator/extension-registry/archive/refs/heads/main.tar.gz"});

    // clang-format off
    setLayouter([this] {
        using namespace Layouting;
        using namespace Core;
        return Column {
            Group {
                title(Tr::tr("Note")),
                Column {
                    Label {
                        wordWrap(true),
                        text(externalRepoWarningNote()),
                    }
                }
            },
            Group {
                title(Tr::tr("Use External Repository")),
                groupChecker(useExternalRepo.groupChecker()),
                Form {
                    repositoryUrls, br,
                },
            },
            Row {
                PushButton {
                    text(Tr::tr("Install Extension...")),
                    onClicked(this, [] {
                        if (executePluginInstallWizard() == InstallResult::NeedsRestart) {
                            ICore::askForRestart(
                                Tr::tr("Plugin changes will take effect after restart."));
                        }
                    }),
                },
                st,
            },
            st,
            spacing(Utils::StyleHelper::SpacingTokens::ExVPaddingGapXl),
        };
    });
    // clang-format on

    readSettings();
}

class ExtensionManagerSettingsPage : public Core::IOptionsPage
{
public:
    ExtensionManagerSettingsPage()
    {
        setId(Constants::EXTENSIONMANAGER_SETTINGSPAGE_ID);
        setDisplayName(Tr::tr("Browser"));
        setCategory(Constants::EXTENSIONMANAGER_SETTINGSPAGE_CATEGORY);
        setSettingsProvider([] { return &settings(); });
    }
};

const ExtensionManagerSettingsPage settingsPage;

QString externalRepoWarningNote()
{
    return
    Tr::tr("%1 does not check extensions from external vendors for security "
           "flaws or malicious intent, so be careful when installing them, "
           "as it might leave your computer vulnerable to attacks such as "
           "hacking, malware, and phishing.")
        .arg(QGuiApplication::applicationDisplayName());
}

} // ExtensionManager::Internal
