// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "settings.h"

#include "mesonpluginconstants.h"
#include "mesonprojectmanagertr.h"

#include <coreplugin/dialogs/ioptionspage.h>

#include <utils/layoutbuilder.h>

namespace MesonProjectManager::Internal {

MesonSettings &settings()
{
    static MesonSettings theSettings;
    return theSettings;
}

MesonSettings::MesonSettings()
{
    setAutoApply(false);
    setSettingsGroup("MesonProjectManager");

    autorunMeson.setSettingsKey("meson.autorun");
    autorunMeson.setLabelText(Tr::tr("Autorun Meson"));
    autorunMeson.setToolTip(Tr::tr("Automatically run Meson when needed."));

    verboseNinja.setSettingsKey("ninja.verbose");
    verboseNinja.setLabelText(Tr::tr("Ninja verbose mode"));
    verboseNinja.setToolTip(Tr::tr("Enables verbose mode by default when invoking Ninja."));

    setLayouter([this] {
        using namespace Layouting;
        return Column {
            autorunMeson,
            verboseNinja,
            st,
        };
    });

    readSettings();
}

class MesonSettingsPage final : public Core::IOptionsPage
{
public:
    MesonSettingsPage()
    {
        setId("A.MesonProjectManager.SettingsPage.General");
        setDisplayName(Tr::tr("General"));
        setDisplayCategory("Meson");
        setCategory(Constants::SettingsPage::CATEGORY);
        setCategoryIconPath(Constants::Icons::MESON_BW);
        setSettingsProvider([] { return &settings(); });
    }
};

const MesonSettingsPage settingsPage;

} // MesonProjectManager::Internal
