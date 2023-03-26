// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import HelperWidgets 2.0
import StudioTheme 1.0 as StudioTheme

SecondColumnLayout {
    property variant bold: backendValues.font_bold
    property variant italic: backendValues.font_italic
    property variant underline: backendValues.font_underline
    property variant strikeout: backendValues.font_strikeout

    BoolButtonRowButton {
        id: boldButton
        buttonIcon: StudioTheme.Constants.fontStyleBold
        backendValue: bold
        enabled: backendValue.isAvailable
    }

    Spacer {
        implicitWidth: StudioTheme.Values.twoControlColumnWidth
                       + StudioTheme.Values.actionIndicatorWidth
                       - (boldButton.implicitWidth + italicButton.implicitWidth)
    }

    BoolButtonRowButton {
        id: italicButton
        buttonIcon: StudioTheme.Constants.fontStyleItalic
        backendValue: italic
        enabled: backendValue.isAvailable
    }

    Spacer { implicitWidth: StudioTheme.Values.twoControlColumnGap }

    BoolButtonRowButton {
        id: underlineButton
        buttonIcon: StudioTheme.Constants.fontStyleUnderline
        backendValue: underline
        enabled: backendValue.isAvailable
    }

    Spacer {
        implicitWidth: StudioTheme.Values.twoControlColumnWidth
                       + StudioTheme.Values.actionIndicatorWidth
                       - (underlineButton.implicitWidth + strikethroughButton.implicitWidth)
    }

    BoolButtonRowButton {
        id: strikethroughButton
        buttonIcon: StudioTheme.Constants.fontStyleStrikethrough
        backendValue: strikeout
        enabled: backendValue.isAvailable
    }

    ExpandingSpacer {}
}
