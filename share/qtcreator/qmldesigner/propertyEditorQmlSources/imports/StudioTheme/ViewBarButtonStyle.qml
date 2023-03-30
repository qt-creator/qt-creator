// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0 WITH Qt-GPL-exception-1.0

import QtQuick

ControlStyle {

    baseIconFontSize: Values.bigFont
    controlSize: Qt.size(Values.viewBarComboWidth, Values.viewBarComboHeight)
    smallIconFontSize: Values.viewBarComboIcon
    borderWidth: Values.border

    radius: Values.smallRadius

    icon: ControlStyle.IconColors {
        idle: Values.themeTextColor
        hover: Values.themeTextColor
        interaction: Values.themeIconColor
        disabled: Values.themeToolbarIcon_blocked
    }

    background: ControlStyle.BackgroundColors {
        idle: Values.themeControlBackground_toolbarIdle
        hover: Values.themeControlBackground_topToolbarHover
        interaction: Values.themeInteraction
        disabled: Values.themeControlBackground_toolbarIdle
    }

    border: ControlStyle.BorderColors {
        idle: Values.themeControlBackground_toolbarIdle
        hover: Values.themeControlBackground_topToolbarHover
        interaction: Values.themeInteraction
        disabled: Values.themeControlBackground_toolbarIdle
    }
}
