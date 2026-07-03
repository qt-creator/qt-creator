// Copyright (C) 2026 Sebastian Mosiej (sebastian.mosiej@wp.pl)
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "zenmodesettings.h"

#include "zenmodeplugintr.h"

#include <coreplugin/dialogs/ioptionspage.h>
#include <coreplugin/helpmanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/modemanager.h>

#include <utils/layoutbuilder.h>
#include <utils/shutdownguard.h>

#include <QGuiApplication>
#include <QLabel>

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
        auto modeSelectorLabel = new QLabel(
            QString("<a href=\""
                    "qthelp://org.qt-project.qtcreator/doc/"
                    "creator-how-to-switch-between-modes.html"
                    "\">%1</a>")
                .arg(Tr::tr("Mode selector:")));
        modeSelectorLabel->setToolTip(
            //: %1=Qt Creator
            Tr::tr(
                "Determines the style to use for the global mode selector in %1 (see View > Modes) "
                "when Zen mode or Distraction Free mode is enabled.")
                .arg(QGuiApplication::applicationDisplayName()));
        QObject::connect(modeSelectorLabel, &QLabel::linkActivated, [](const QString &link) {
            HelpManager::showHelpUrl(link, HelpManager::ExternalHelpAlways);
        });
        // clang-format off
        return Column {
            Group {
                title(Tr::tr("When Zen Mode or Distraction Free Mode Is Active")),
                Column {
                    Row { contentWidth, st, },
                    Row { modeSelectorLabel, modes, st}
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
