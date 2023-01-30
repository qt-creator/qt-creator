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
        hover: Values.themeStateControlBackgroundColor_hover
        globalHover: Values.themeStateControlBackgroundColor_globalHover
        interaction: Values.themeInteraction
    }

    border: ControlStyle.BorderColors {
        hover: Values.themeControlBackground_toolbarHover
        interaction: Values.themeInteraction
    }
}
