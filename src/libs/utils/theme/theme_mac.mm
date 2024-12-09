// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "theme_mac.h"

#include <QMenu>

#include <AppKit/AppKit.h>

namespace Utils {
namespace Internal {

void forceMacAppearance(bool dark)
{
    NSApp.appearance = [NSAppearance
        appearanceNamed:(dark ? NSAppearanceNameDarkAqua : NSAppearanceNameAqua)];
}

void setMacOSHelpMenu(QMenu *menu)
{
    NSApp.helpMenu = menu->toNSMenu();
}

} // Internal
} // Utils
