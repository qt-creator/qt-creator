// Copyright (C) 2026 Sebastian Mosiej (sebastian.mosiej@wp.pl)
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "zenmodesettings.h"

#include "zenmodeplugintr.h"

#include <coreplugin/dialogs/ioptionspage.h>
#include <coreplugin/icore.h>
#include <coreplugin/modemanager.h>

#include <utils/layoutbuilder.h>
#include <utils/shutdownguard.h>

using namespace Core;
using namespace Utils;

namespace ZenMode {

ZenModeSettings &settings()
{
    static GuardedObject<ZenModeSettings> theSettings;
    return theSettings;
}

ZenModeSettings::ZenModeSettings()
{
    setSettingsGroup("ZenMode");
    setAutoApply(false);

    contentWidth.setSettingsKey("EditorContentWidth");
    contentWidth.setSuffix("%");
    contentWidth.setLabelText(Tr::tr("Editor content width:"));
    contentWidth.setDefaultValue(100);
    contentWidth.setRange(50, 100);

    modes.setSettingsKey("ModesBarState");
    SelectionAspect::Option optHidden(
        Tr::tr("Hidden"),
        Tr::tr("Hide the mode selector (default)."),
        int(ModeManager::Style::Hidden));
    SelectionAspect::Option optIconsOnly(
        Tr::tr("Icons Only"),
        Tr::tr("Show only icons in the mode selector."),
        int(ModeManager::Style::IconsOnly));
    SelectionAspect::Option optIconsText(
        Tr::tr("Icons and Text"),
        Tr::tr("Show icons and text in the mode selector."),
        int(ModeManager::Style::IconsAndText));
    modes.addOption(optHidden);
    modes.addOption(optIconsOnly);
    modes.addOption(optIconsText);

    setLayouter([this] {
        using namespace Layouting;
        // clang-format off
        return Column {
            Group {
                title(Tr::tr("When Zen Mode is Active")),
                Column {
                    Row { contentWidth, st, },
                    Row { Tr::tr("Mode selector:"), modes, st}
                }
            },
            st
        };
        // clang-format on
    });

    readSettings();
}

class ZenModeSettingsPage final : public IOptionsPage
{
public:
    ZenModeSettingsPage()
    {
        setId("ZenMode.General");
        setDisplayName("Zen Mode");
        setCategory("ZY.ZenMode");
        setSettingsProvider([] { return &settings(); });
    }
};

const ZenModeSettingsPage settingsPage;

} // namespace ZenMode
