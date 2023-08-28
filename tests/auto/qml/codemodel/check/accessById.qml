// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Window

Window {
    id: root
    width: 640
    height: 480
    visible: true
    title: qsTr("Hello World")

    Rectangle {
        id: rect

        Rectangle {
            id: innerRect
        }
    }

    Text {
        id: myText
        width: 50
        wrapMode: Text.WordWrap
        text: "a text string that is longer than 50 pixels"

        Text {
            id: innerText
        }
        states: [
            State {
                name: "widerText"
                PropertyChanges { myText.width: undefined }
                AnchorChanges { innerRect.width: undefined } // 16 29 37
            },
            State {
                when: root.visible
                PropertyChanges {
                    // change an object property that is not an ancestor
                    innerRect {
                        color: "blue"
                    }
                }
            }
        ]
    }

    Binding {rect.width: innerText.width}
}
