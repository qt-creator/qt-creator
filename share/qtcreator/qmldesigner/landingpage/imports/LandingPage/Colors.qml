// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

pragma Singleton
import QtQuick 2.15
import LandingPageTheme

QtObject {
    readonly property color text: Theme.color(Theme.Token_Text_Default)
    readonly property color foregroundPrimary: Theme.color(Theme.Token_Foreground_Default)
    readonly property color foregroundSecondary: Theme.color(Theme.Token_Foreground_Muted)
    readonly property color backgroundPrimary: Theme.color(Theme.Token_Background_Default)
    readonly property color backgroundSecondary: Theme.color(Theme.Token_Background_Muted)
    readonly property color hover: Theme.color(Theme.Token_Background_Subtle)
    readonly property color accent: Theme.color(Theme.Token_Accent_Default)
    readonly property color link: Theme.color(Theme.Token_Text_Accent)
    readonly property color disabledLink: Theme.color(Theme.Token_Text_Subtle)
}
