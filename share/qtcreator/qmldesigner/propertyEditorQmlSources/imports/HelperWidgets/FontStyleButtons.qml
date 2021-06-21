/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

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
