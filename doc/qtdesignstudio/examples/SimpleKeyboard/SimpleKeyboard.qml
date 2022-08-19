// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick 2.15
import SimpleKeyboard 1.0
import QtQuick.VirtualKeyboard 2.15
import QtQuick.Window 2.2

Window {
    id: window
    width: Constants.width
    height: Constants.height
    Item {
        id: cornerItem
        x: 0
        y: 0
    }

    property int activeFocusItemBottom : activeFocusItem == null ? 0 : Math.min(height, cornerItem.mapFromItem(activeFocusItem, 0, activeFocusItem.height).y + 50)

    Swiper {
        id: view
        width: window.width
        height: window.height
        y: !inputPanel.active ? 0 : Math.min(0, window.height - inputPanel.height - activeFocusItemBottom)
        Behavior on y {
            NumberAnimation {
                duration: 250
                easing.type: Easing.InOutQuad
            }
        }
    }

    Rectangle {
        id: inputPanelBackground
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: inputPanel.top
        anchors.bottom: inputPanel.bottom
        color: "black"
    }

    InputPanel {
        id: inputPanel
        x: window.width/2 - width/2
        y: window.height
        width: Math.min(window.width, window.height)

        states: State {
            name: "visible"
            when: inputPanel.active
            PropertyChanges {
                target: inputPanel
                y: window.height - inputPanel.height
            }
        }
        transitions: Transition {
            from: ""
            to: "visible"
            reversible: true
            ParallelAnimation {
                NumberAnimation {
                    properties: "y"
                    duration: 250
                    easing.type: Easing.InOutQuad
                }
            }
        }
    }
}

