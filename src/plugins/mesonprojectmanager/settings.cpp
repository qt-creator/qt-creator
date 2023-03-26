// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "settings.h"

#include "mesonpluginconstants.h"
#include "mesonprojectmanagertr.h"

#include <utils/layoutbuilder.h>

namespace MesonProjectManager {
namespace Internal {

Settings::Settings()
{
    setSettingsGroup("MesonProjectManager");
    setAutoApply(false);

    autorunMeson.setSettingsKey("meson.autorun");
    autorunMeson.setLabelText(Tr::tr("Autorun Meson"));
    autorunMeson.setToolTip(Tr::tr("Automatically run Meson when needed."));

    verboseNinja.setSettingsKey("ninja.verbose");
    verboseNinja.setLabelText(Tr::tr("Ninja verbose mode"));
    verboseNinja.setToolTip(Tr::tr("Enables verbose mode by default when invoking Ninja."));

    registerAspect(&autorunMeson);
    registerAspect(&verboseNinja);
}

Settings *Settings::instance()
{
    static Settings m_settings;
    return &m_settings;
}

GeneralSettingsPage::GeneralSettingsPage()
{
    setId(Constants::SettingsPage::GENERAL_ID);
    setDisplayName(Tr::tr("General"));
    setDisplayCategory("Meson");
    setCategory(Constants::SettingsPage::CATEGORY);
    setCategoryIconPath(Constants::Icons::MESON_BW);
    setSettings(Settings::instance());

    setLayouter([](QWidget *widget) {
        Settings &s = *Settings::instance();
        using namespace Utils::Layouting;

        Column {
            s.autorunMeson,
            s.verboseNinja,
            st,
        }.attachTo(widget);
    });
}

} // namespace Internal
} // namespace MesonProjectManager
