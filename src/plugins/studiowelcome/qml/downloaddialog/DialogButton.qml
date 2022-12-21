// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.10
import QtQuick.Templates 2.1 as T

T.Button {
    id: control

    implicitWidth: Math.max(buttonBackground ? buttonBackground.implicitWidth : 0,
                                         textItem.implicitWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(buttonBackground ? buttonBackground.implicitHeight : 0,
                                          textItem.implicitHeight + topPadding + bottomPadding)
    leftPadding: 4
    rightPadding: 4

    text: "My Button"

    property color defaultColor: "#b9b9ba"
    property color checkedColor:  "#ffffff"

    background: buttonBackground
    Rectangle {
        id: buttonBackground
        color: control.defaultColor
        implicitWidth: 100
        implicitHeight: 40
        opacity: enabled ? 1 : 0.3
        radius: 0
        border.width: 1
    }

    contentItem: textItem
    Text {
        id: textItem
        text: control.text

        opacity: enabled ? 1.0 : 0.3
        color: "#bababa"
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }

    states: [
        State {
            name: "normal"
            when: !control.down && !control.checked
            PropertyChanges {
                target: buttonBackground
                color:  "#2d2e30"
            }
        },
        State {
            name: "down"
            when: control.down || control.checked
            PropertyChanges {
                target: textItem
                color: control.checkedColor
            }

            PropertyChanges {
                target: buttonBackground
                color: "#545456"
                border.color: "#70a2f5"
                border.width: 2
            }
        }
    ]
}
