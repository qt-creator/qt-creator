// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "terminalsettings.h"

#include "terminaltr.h"

#include <utils/environment.h>
#include <utils/hostosinfo.h>

using namespace Utils;

namespace Terminal {

static QString defaultFontFamily()
{
    if (HostOsInfo::isMacHost())
        return QLatin1String("Menlo");

    if (Utils::HostOsInfo::isAnyUnixHost())
        return QLatin1String("Monospace");

    return QLatin1String("Consolas");
}

static int defaultFontSize()
{
    if (Utils::HostOsInfo::isMacHost())
        return 12;
    if (Utils::HostOsInfo::isAnyUnixHost())
        return 9;
    return 10;
}

static QString defaultShell()
{
    if (Utils::HostOsInfo::isMacHost())
        return "/bin/zsh";
    if (Utils::HostOsInfo::isAnyUnixHost())
        return "/bin/bash";
    return qtcEnvironmentVariable("COMSPEC");
}

TerminalSettings &TerminalSettings::instance()
{
    static TerminalSettings settings;
    return settings;
}

void setupColor(ColorAspect &color, const QString &label, const QColor &defaultColor)
{
    color.setSettingsKey(label);
    color.setDefaultValue(defaultColor);
    color.setToolTip(Tr::tr("The color used for %1.").arg(label));
}

TerminalSettings::TerminalSettings()
{
    setAutoApply(false);
    setSettingsGroup("Terminal");

    font.setSettingsKey("FontFamily");
    font.setLabelText(Tr::tr("Family:"));
    font.setHistoryCompleter("Terminal.Fonts.History");
    font.setToolTip(Tr::tr("The font family used in the terminal."));
    font.setDefaultValue(defaultFontFamily());

    fontSize.setSettingsKey("FontSize");
    fontSize.setLabelText(Tr::tr("Size:"));
    fontSize.setToolTip(Tr::tr("The font size used in the terminal. (in points)"));
    fontSize.setDefaultValue(defaultFontSize());
    fontSize.setRange(1, 100);

    shell.setSettingsKey("ShellPath");
    shell.setLabelText(Tr::tr("Shell path:"));
    shell.setExpectedKind(PathChooser::ExistingCommand);
    shell.setDisplayStyle(StringAspect::PathChooserDisplay);
    shell.setHistoryCompleter("Terminal.Shell.History");
    shell.setToolTip(Tr::tr("The shell executable to be started as terminal"));
    shell.setDefaultValue(defaultShell());

    setupColor(foregroundColor, "Foreground", QColor::fromRgb(0xff, 0xff, 0xff));
    setupColor(backgroundColor, "Background", QColor::fromRgb(0x0, 0x0, 0x0));
    setupColor(selectionColor, "Selection", QColor::fromRgb(0xFF, 0xFF, 0xFF, 0x7F));

    setupColor(colors[0], "0", QColor::fromRgb(0x00, 0x00, 0x00));
    setupColor(colors[8], "8", QColor::fromRgb(102, 102, 102));

    setupColor(colors[1], "1", QColor::fromRgb(139, 27, 16));
    setupColor(colors[9], "9", QColor::fromRgb(210, 45, 31));

    setupColor(colors[2], "2", QColor::fromRgb(74, 163, 46));
    setupColor(colors[10], "10", QColor::fromRgb(98, 214, 63));

    setupColor(colors[3], "3", QColor::fromRgb(154, 154, 47));
    setupColor(colors[11], "11", QColor::fromRgb(229, 229, 75));

    setupColor(colors[4], "4", QColor::fromRgb(0, 0, 171));
    setupColor(colors[12], "12", QColor::fromRgb(0, 0, 254));

    setupColor(colors[5], "5", QColor::fromRgb(163, 32, 172));
    setupColor(colors[13], "13", QColor::fromRgb(210, 45, 222));

    setupColor(colors[6], "6", QColor::fromRgb(73, 163, 176));
    setupColor(colors[14], "14", QColor::fromRgb(105, 226, 228));

    setupColor(colors[7], "7", QColor::fromRgb(191, 191, 191));
    setupColor(colors[15], "15", QColor::fromRgb(229, 229, 230));

    registerAspect(&font);
    registerAspect(&fontSize);
    registerAspect(&shell);

    registerAspect(&foregroundColor);
    registerAspect(&backgroundColor);

    for (auto &color : colors)
        registerAspect(&color);
}

} // namespace Terminal
