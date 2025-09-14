// Copyright (C) 2026 Sebastian Mosiej (sebastian.mosiej@wp.pl)
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "zenmodesettings.h"

#include "zenmodeplugintr.h"

#include <coreplugin/icore.h>
#include <coreplugin/dialogs/ioptionspage.h>

#include <utils/layoutbuilder.h>
#include <utils/shutdownguard.h>

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

    modes.setLabelText(Tr::tr("Modes bar state when active"));
    modes.setSettingsKey("ModesBarState");
    SelectionAspect::Option optHidden(Tr::tr("Hidden"),
        Tr::tr("Hide Modes bar (default)."), {0});
    SelectionAspect::Option optIconsOnly(Tr::tr("Icons only"),
        Tr::tr("Show Modes bar icons only"), {1});
    SelectionAspect::Option optIconsText(Tr::tr("Icons and Text"),
        Tr::tr("Show Modes bar icons and text"), {2});
    modes.addOption(optHidden);
    modes.addOption(optIconsOnly);
    modes.addOption(optIconsText);

    setLayouter([this] {
        using namespace Layouting;
        // clang-format off
        return Column {
            Group {
                title(Tr::tr("When ZenMode is active")),
                Column {
                    Row { contentWidth, st, },
                    Group {
                        title(Tr::tr("Mode")),
                        Row { modes, st}
                    },
                }
            },
            st
        };
        // clang-format on
    });

    readSettings();
}

class ZenModeSettingsPage final : public Core::IOptionsPage
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
