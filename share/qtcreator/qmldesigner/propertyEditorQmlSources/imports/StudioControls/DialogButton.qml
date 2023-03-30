// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0 WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Templates as T
import StudioTheme 1.0 as StudioTheme

T.Button {
    id: control

    property StudioTheme.ControlStyle style: StudioTheme.Values.controlStyle

    implicitWidth: Math.max(buttonBackground ? buttonBackground.implicitWidth : 0,
                            textItem.implicitWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(buttonBackground ? buttonBackground.implicitHeight : 0,
                             textItem.implicitHeight + topPadding + bottomPadding)
    leftPadding: control.style.dialogPadding
    rightPadding: control.style.dialogPadding

    background: Rectangle {
        id: buttonBackground
        implicitWidth: 70
        implicitHeight: 20
        color: control.style.background.idle
        border.color: control.style.border.idle
        anchors.fill: parent
    }

    contentItem: Text {
        id: textItem
        text: control.text
        font.pixelSize: control.style.baseFontSize
        color: control.style.text.idle
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }

    states: [
        State {
            name: "default"
            when: control.enabled && !control.down && !control.hovered && !control.checked

            PropertyChanges {
                target: buttonBackground
                color: control.highlighted ? control.style.interaction
                                           : control.style.background.idle
                border.color: control.style.border.idle
            }
            PropertyChanges {
                target: textItem
                color: control.style.text.idle
            }
        },
        State {
            name: "hover"
            when: control.enabled && control.hovered && !control.checked && !control.down

            PropertyChanges {
                target: buttonBackground
                color: control.style.background.hover
                border.color: control.style.border.hover
            }
            PropertyChanges {
                target: textItem
                color: control.style.text.hover
            }
        },
        State {
            name: "press"
            when: control.enabled && (control.checked || control.down)

            PropertyChanges {
                target: buttonBackground
                color: control.style.background.interaction
                border.color: control.style.border.interaction
            }
            PropertyChanges {
                target: textItem
                color: control.style.text.interaction
            }
        },
        State {
            name: "disable"
            when: !control.enabled
            PropertyChanges {
                target: buttonBackground
                color: control.style.background.disabled
                border.color: control.style.border.disabled
            }
            PropertyChanges {
                target: textItem
                color: control.style.text.disabled
            }
        }
    ]
}
