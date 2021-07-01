/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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
import QtQuick.Templates 2.15 as T
import StudioTheme 1.0 as StudioTheme

T.AbstractButton {
    id: myButton

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    background: Rectangle {
        id: buttonBackground
        implicitWidth: 100
        implicitHeight: 23
        radius: 3
        color: StudioTheme.Values.themeControlBackground
        border.color: StudioTheme.Values.themeControlOutline
        border.width: StudioTheme.Values.border
    }

    contentItem: Text {
        id: buttonText
        color: StudioTheme.Values.themeTextColor
        font.family: StudioTheme.Constants.font.family
        font.pixelSize: StudioTheme.Values.myFontSize
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter
        anchors.fill: parent
        renderType: Text.QtRendering
        text: myButton.text
    }

    states: [
        State {
            name: "default"
            when: myButton.enabled && !myButton.hovered && !myButton.pressed
                  && !myButton.checked
            PropertyChanges {
                target: buttonBackground
                color: StudioTheme.Values.themeControlBackground
            }
        },
        State {
            name: "hovered"
            when: myButton.enabled && myButton.hovered && !myButton.pressed
            PropertyChanges {
                target: buttonBackground
                color: StudioTheme.Values.themeControlBackgroundHover
            }
        },
        State {
            name: "pressed"
            when: myButton.enabled && myButton.hovered && myButton.pressed
            PropertyChanges {
                target: buttonBackground
                color: StudioTheme.Values.themeControlBackgroundInteraction
                border.color: StudioTheme.Values.themeControlOutlineInteraction
            }
        },
        State {
            name: "disabled"
            when: !myButton.enabled
            PropertyChanges {
                target: buttonBackground
                color: StudioTheme.Values.themeControlBackgroundDisabled
                border.color: StudioTheme.Values.themeControlOutlineDisabled
            }
            PropertyChanges {
                target: buttonText
                color: StudioTheme.Values.themeTextColorDisabled
            }
        }
    ]
}
