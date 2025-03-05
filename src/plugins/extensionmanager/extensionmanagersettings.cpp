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

    externalRepoUrl.setSettingsKey("ExternalRepoUrl");
    externalRepoUrl.setDefaultValue("https://qc-extensions.qt.io");
    externalRepoUrl.setDisplayStyle(Utils::StringAspect::LineEditDisplay);
    externalRepoUrl.setLabelText(Tr::tr("Server URL:"));

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
                    externalRepoUrl
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
