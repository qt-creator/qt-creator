// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

pragma Singleton
import QtQuick 2.15
import LandingPageTheme

QtObject {
    readonly property color text: Theme.color(Theme.Welcome_TextColor)
    readonly property color foregroundPrimary: Theme.color(Theme.Welcome_ForegroundPrimaryColor)
    readonly property color foregroundSecondary: Theme.color(Theme.Welcome_ForegroundSecondaryColor)
    readonly property color backgroundPrimary: Theme.color(Theme.Welcome_BackgroundPrimaryColor)
    readonly property color backgroundSecondary: Theme.color(Theme.Welcome_BackgroundSecondaryColor)
    readonly property color hover: Theme.color(Theme.Welcome_HoverColor)
    readonly property color accent: Theme.color(Theme.Welcome_AccentColor)
    readonly property color link: Theme.color(Theme.Welcome_LinkColor)
    readonly property color disabledLink: Theme.color(Theme.Welcome_DisabledLinkColor)
}
