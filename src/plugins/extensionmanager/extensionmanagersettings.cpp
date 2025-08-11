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

#ifndef QT_NO_SSL
  #include <QSslSocket>
#endif

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

#ifndef QT_NO_SSL
    const bool sslSupported = QSslSocket::supportsSsl();
#else
    const bool sslSupported = false;
#endif

    useExternalRepo.setEnabled(sslSupported);
    if (!sslSupported) {
        useExternalRepo.setToolTip(Tr::tr("SSL support is not available."));
    }

    repositoryUrls.setSettingsKey("RepositoryUrls");
    repositoryUrls.setLabelText(Tr::tr("Repository URLs:"));
    repositoryUrls.setToolTip(
        Tr::tr("Repositories to query for extensions. You can specify local paths or "
               "HTTP(S) URLs that should be merged with the main repository."));
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
            spacing(Utils::StyleHelper::SpacingTokens::GapVXxl),
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
    Tr::tr("If you choose to link or connect an external repository, "
           "you are acting at your own discretion and risk. "
           "The Qt Company does not control, endorse, or maintain any "
           "external repositories that you connect. Any changes, "
           "unavailability or security issues in external repositories "
           "are beyond The Qt Company's control and responsibility. "
           "By linking or connecting external repositories, you "
           "acknowledge these conditions and accept responsibility "
           "for managing associated risks appropriately.");
}

} // ExtensionManager::Internal
