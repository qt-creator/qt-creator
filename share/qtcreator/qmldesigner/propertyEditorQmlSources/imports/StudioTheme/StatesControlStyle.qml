// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0 WITH Qt-GPL-exception-1.0

import QtQuick

ControlStyle {
    radius: Values.smallRadius
    baseIconFontSize: Values.baseFont
    controlSize: Qt.size(Values.viewBarComboWidth, Values.viewBarComboHeight)
    smallIconFontSize: Values.viewBarComboIcon

    background: ControlStyle.BackgroundColors {
        idle: Values.themeToolbarBackground
        hover: Values.themeStateControlBackgroundColor_globalHover
        globalHover: Values.themeStateControlBackgroundColor_globalHover
        interaction: Values.themeToolbarBackground
    }
    text: ControlStyle.TextColors {
        idle: Values.themeTextColor
        interaction: Values.themeTextSelectedTextColor
        hover: Values.themeTextColor
        disabled: Values.themeTextColorDisabled
        selection: Values.themeTextSelectionColor
        selectedText: Values.themeTextSelectedTextColor
        placeholder: Values.themeTextColor
        placeholderInteraction: Values.themeTextColor
    }
    border: ControlStyle.BorderColors {
        idle: Values.controlOutline_toolbarIdle
        hover: Values.themeStateHighlight
        interaction: Values.themeInteraction
    }
}
