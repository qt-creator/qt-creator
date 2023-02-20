// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0 WITH Qt-GPL-exception-1.0

import QtQuick

ControlStyle {

    baseIconFontSize: Values.bigFont
    controlSize: Qt.size(Values.viewBarComboWidth, Values.viewBarComboHeight)
    smallIconFontSize: Values.viewBarComboIcon

    radius: Values.smallRadius

    icon: ControlStyle.IconColors {
        idle: Values.themeTextColor
        hover: Values.themeTextColor
        interaction: Values.themeIconColor
        disabled: "#636363"
    }

    background: ControlStyle.BackgroundColors {
        idle: Values.themecontrolBackground_statusbarIdle
        hover: Values.themecontrolBackground_statusbarHover
        interaction: Values.themeInteraction
        disabled: Values.themecontrolBackground_statusbarIdle
    }

    border: ControlStyle.BorderColors {
        idle: Values.themecontrolBackground_statusbarIdle
        hover: Values.themeControlBackground_toolbarHover
        interaction: Values.themeInteraction
        disabled: Values.themecontrolBackground_statusbarIdle
    }
}
