// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0 WITH Qt-GPL-exception-1.0

import QtQuick

ControlStyle {
    radius: Values.smallRadius
    baseIconFontSize: Values.miniIcon
    controlSize: Qt.size(Values.viewBarComboWidth, Values.viewBarComboHeight)
    smallIconFontSize: Values.viewBarComboIcon

    background: ControlStyle.BackgroundColors {
        idle: Values.themeToolbarBackground
        hover: Values.themeControlBackground_toolbarHover
        globalHover: Values.themeControlBackground_toolbarHover
        interaction: Values.themeToolbarBackground
    }

    text: ControlStyle.TextColors {
        idle: Values.themeTextColor
        interaction: Values.themeTextSelectedTextColor
        hover: Values.themeTextColor
        disabled: Values.themeToolbarIcon_blocked
        selection: Values.themeTextSelectionColor
        selectedText: Values.themeTextSelectedTextColor
        //placeholder: Values.themeTextColorDisabled
        //placeholderHover: Values.themeTextColor
        placeholderInteraction: Values.themeTextColor
    }

    border: ControlStyle.BorderColors {
        idle: Values.controlOutline_toolbarIdle
        hover: Values.controlOutline_toolbarHover
        interaction: Values.themeInteraction
    }

    icon: ControlStyle.IconColors {
        idle: Values.themeIconColor
        interaction: Values.themeIconColorInteraction
        selected: Values.themeIconColorSelected
        hover: Values.themeIconColorHover
        disabled: Values.themeToolbarIcon_blocked
    }
}
