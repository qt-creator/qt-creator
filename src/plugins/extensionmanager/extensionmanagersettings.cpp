// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "extensionmanagersettings.h"
#include "extensionmanagertr.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/dialogs/ioptionspage.h>

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

    externalRepoUrl.setDefaultValue("https://qc-extensions.qt.io");
    externalRepoUrl.setReadOnly(true);

    useExternalRepo.setSettingsKey("UseExternalRepo");
    useExternalRepo.setLabelText(Tr::tr("Use external repository"));
    useExternalRepo.setToolTip(Tr::tr("Repository: %1").arg(externalRepoUrl()));
    useExternalRepo.setDefaultValue(false);

    setLayouter([this] {
        using namespace Layouting;

        return Column {
            useExternalRepo,
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
        setId("ExtensionManager");
        setDisplayName(Tr::tr("Extensions"));
        setCategory(Core::Constants::SETTINGS_CATEGORY_CORE);
        setSettingsProvider([] { return &settings(); });
    }
};

const ExtensionManagerSettingsPage settingsPage;

} // ExtensionManager::Internal
