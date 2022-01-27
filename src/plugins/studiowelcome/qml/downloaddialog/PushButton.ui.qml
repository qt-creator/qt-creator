

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
import QtQuick.Templates 2.15
import ExampleCheckout 1.0
import StudioTheme 1.0

Button {
    id: control

    implicitWidth: Math.max(
                       buttonBackground ? buttonBackground.implicitWidth : 0,
                       textItem.implicitWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(
                        buttonBackground ? buttonBackground.implicitHeight : 0,
                        textItem.implicitHeight + topPadding + bottomPadding)
    leftPadding: 4
    rightPadding: 4

    text: "My Button"
    property alias fontpixelSize: textItem.font.pixelSize
    //property bool decorated: false
    state: "normal"

    background: buttonBackground
    Rectangle {
        id: buttonBackground
        color: "#00000000"
        implicitWidth: 100
        implicitHeight: 40
        opacity: enabled ? 1 : 0.3
        radius: 2
        border.color: "#047eff"
        anchors.fill: parent
    }

    contentItem: textItem

    Text {
        id: textItem
        text: control.text
        font.pixelSize: 18

        opacity: enabled ? 1.0 : 0.3
        color: Values.themeTextColor
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }

    states: [
        State {
            name: "normal"
            when: !control.down && !control.hovered

            PropertyChanges {
                target: buttonBackground
                color: Values.themeControlBackground
                border.color: Values.themeControlOutline
            }

            PropertyChanges {
                target: textItem
                color: Values.themeTextColor
            }
        },
        State {
            name: "hover"
            when: control.hovered && !control.down
            PropertyChanges {
                target: textItem
                color: Values.themeTextColor
            }

            PropertyChanges {
                target: buttonBackground
                color: Values.themeControlBackgroundHover
                border.color: Values.themeControlOutline
            }
        },
        State {
            name: "activeQds"
            when: control.down
            PropertyChanges {
                target: textItem
                color: "#ffffff"
            }

            PropertyChanges {
                target: buttonBackground
                color: "#2e769e"
                border.color: "#00000000"
            }
        }
    ]
}
