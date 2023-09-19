// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0 WITH Qt-GPL-exception-1.0

import QtQuick

ControlStyle {
    radius: Values.smallRadius

    baseIconFontSize: Values.baseFont
    controlSize: Qt.size(Values.viewBarComboWidth, Values.viewBarComboHeight)
    smallIconFontSize: Values.baseFont

    scrollBarThickness: 4
    scrollBarThicknessHover: 6

    background: ControlStyle.BackgroundColors {
        idle: Values.themePopoutControlBackground_idle
        hover: Values.themePopoutControlBackground_hover
        globalHover: Values.themePopoutControlBackground_globalHover
        interaction: Values.themeControlBackgroundInteraction
        disabled: Values.themePopoutControlBackground_disabled
    }

    popup: ControlStyle.PopupColors {
        background: Values.themePopoutPopupBackground
    }

    icon: ControlStyle.IconColors {
        idle: Values.themeTextColor
        hover: Values.themeTextColor
        interaction: Values.themeTextSelectedTextColor
        disabled: Values.themeTextColorDisabled
    }

    border: ControlStyle.BorderColors {
        idle: Values.themePopoutControlBorder_idle
        hover: Values.themePopoutControlBorder_hover
        interaction: Values.themeInteraction
        disabled: Values.themePopoutControlBorder_disabled
    }

    scrollBar: ControlStyle.ScrollBarColors {
        track: Values.themeScrollBarTrack
        handle: Values.themeScrollBarHandle_idle
        handleHover: Values.themeScrollBarHandle
    }
}
