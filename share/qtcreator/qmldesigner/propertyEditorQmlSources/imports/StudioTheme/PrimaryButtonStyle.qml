// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0 WITH Qt-GPL-exception-1.0

import QtQuick

ControlStyle {
    controlSize: Qt.size(Values.topLevelComboWidth, Values.topLevelComboHeight)
    baseIconFontSize: Values.baseFontSize
    radius: Values.smallRadius

    icon: ControlStyle.IconColors {
        idle: Values.themeTextSelectedTextColor
        hover: Values.themeTextSelectedTextColor
        disabled: Values.themeToolbarIcon_blocked
    }

    background: ControlStyle.BackgroundColors {
        idle: Values.themeInteraction
        hover: Values.themePrimaryButton_hoverHighlight
        interaction: Values.themeInteraction
        disabled: Values.themeInteraction
    }

    border: ControlStyle.BorderColors {
        idle: Values.themeInteraction
        hover: Values.themePrimaryButton_hoverHighlight
        interaction: Values.themePrimaryButton_hoverHighlight
        disabled: Values.themeToolbarIcon_blocked
    }
}
