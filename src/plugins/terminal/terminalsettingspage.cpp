// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "terminalsettingspage.h"

#include "terminalsettings.h"
#include "terminaltr.h"

#include <coreplugin/icore.h>

#include <utils/layoutbuilder.h>
#include <utils/pathchooser.h>

#include <QFontComboBox>

using namespace Utils;

namespace Terminal {

TerminalSettingsPage::TerminalSettingsPage()
{
    setId("Terminal.General");
    setDisplayName("Terminal");
    setCategory("ZY.Terminal");
    setDisplayCategory("Terminal");
    setSettings(&TerminalSettings::instance());

    // TODO: Add real icon!
    setCategoryIconPath(":/texteditor/images/settingscategory_texteditor.png");
}

void TerminalSettingsPage::init() {}

QWidget *TerminalSettingsPage::widget()
{
    QWidget *widget = new QWidget;

    using namespace Layouting;

    QFontComboBox *fontComboBox = new QFontComboBox(widget);
    fontComboBox->setFontFilters(QFontComboBox::MonospacedFonts);
    fontComboBox->setCurrentFont(TerminalSettings::instance().font.value());

    connect(fontComboBox, &QFontComboBox::currentFontChanged, this, [](const QFont &f) {
        TerminalSettings::instance().font.setValue(f.family());
    });

    TerminalSettings &settings = TerminalSettings::instance();

    // clang-format off
    Column {
        Group {
            title(Tr::tr("Font")),
            Row {
                settings.font.labelText(), fontComboBox, Space(20),
                settings.fontSize, st,
            },
        },
        Group {
            title(Tr::tr("Colors")),
            Column {
                Row {
                    Tr::tr("Foreground"), settings.foregroundColor, st,
                    Tr::tr("Background"), settings.backgroundColor, st,
                    Tr::tr("Selection"), settings.selectionColor, st,
                },
                Row {
                    settings.colors[0], settings.colors[1],
                    settings.colors[2], settings.colors[3],
                    settings.colors[4], settings.colors[5],
                    settings.colors[6], settings.colors[7]
                },
                Row {
                    settings.colors[8], settings.colors[9],
                    settings.colors[10], settings.colors[11],
                    settings.colors[12], settings.colors[13],
                    settings.colors[14], settings.colors[15]
                },
            }
        },
        Row {
            settings.shell,
        },
        st,
    }.attachTo(widget);
    // clang-format on

    return widget;
}

TerminalSettingsPage &TerminalSettingsPage::instance()
{
    static TerminalSettingsPage settingsPage;
    return settingsPage;
}

} // namespace Terminal
