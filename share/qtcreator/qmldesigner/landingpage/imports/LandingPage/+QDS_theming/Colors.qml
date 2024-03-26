// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

pragma Singleton
import QtQuick 2.15

QtObject {
    readonly property color text:                   "#fff8f8f8" // Token_Text_Default
    readonly property color foregroundPrimary:      "#ff474747" // Token_Foreground_Default
    readonly property color foregroundSecondary:    "#ff353535" // Token_Foreground_Muted
    readonly property color backgroundPrimary:      "#ff1f1f1f" // Token_Background_Default
    readonly property color backgroundSecondary:    "#ff262626" // Token_Background_Muted
    readonly property color hover:                  "#ff2e2e2e" // Token_Background_Subtle
    readonly property color accent:                 "#ff1f9b5d" // Token_Accent_Default
    readonly property color link:                   "#ff23b26a" // Token_Text_Accent
    readonly property color disabledLink:           "#ff595959" // Token_Text_Subtle
}
