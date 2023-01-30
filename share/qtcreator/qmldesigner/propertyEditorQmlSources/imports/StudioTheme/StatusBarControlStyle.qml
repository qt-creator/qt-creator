// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0 WITH Qt-GPL-exception-1.0

import QtQuick

ControlStyle {

    radius: Values.smallRadius

    baseIconFontSize: Values.baseFont
    controlSize: Qt.size(Values.viewBarComboWidth, Values.viewBarComboHeight)
    smallIconFontSize: Values.baseFont

    background: ControlStyle.BackgroundColors {
        idle: Values.themecontrolBackground_statusbarIdle
        hover: Values.themecontrolBackground_statusbarHover
        globalHover: Values.themecontrolBackground_statusbarHover
        interaction: Values.themeInteraction
        disabled: Values.themecontrolBackground_statusbarIdle
    }
    icon: ControlStyle.IconColors {
        idle: Values.themeTextColor
        hover: Values.themeTextColor
        interaction: Values.themeTextSelectedTextColor
        disabled: Values.themeTextColorDisabled
    }

    border: ControlStyle.BorderColors {
        hover: Values.themeControlBackground_toolbarHover
        interaction: Values.themeInteraction
        disabled: Values.themecontrolBackground_statusbarIdle
    }
}
