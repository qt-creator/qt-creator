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

    externalRepoUrl.setSettingsKey("ExternalRepoUrl");
    externalRepoUrl.setDefaultValue("https://qc-extensions.qt.io");
    externalRepoUrl.setDisplayStyle(Utils::StringAspect::LineEditDisplay);
    externalRepoUrl.setLabelText(Tr::tr("Server URL:"));

    setLayouter([this] {
        using namespace Layouting;
        return Column {
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
                    onClicked([] {
                        if (Core::executePluginInstallWizard())
                            Core::ICore::askForRestart(
                                    Tr::tr("Plugin changes will take effect after restart."));
                    }, this),
                },
                st,
            },
            st
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
        setDisplayCategory(Tr::tr("Extensions"));
        setCategoryIconPath(":/extensionmanager/images/settingscategory_extensionmanager.png");
        setSettingsProvider([] { return &settings(); });
    }
};

const ExtensionManagerSettingsPage settingsPage;

} // ExtensionManager::Internal
