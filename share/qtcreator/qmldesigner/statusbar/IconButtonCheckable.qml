// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0
import QtQuick
import QtQuick.Controls
import QtQuick.Templates as T

T.Button {
    id: root
    width: 29
    height: 29
    state: "idle"
    property alias imageSource: image.source
    property color idleBack: "#202020"
    property color hoverBack: "#2d2d2d"

    Rectangle {
        id: bkg
        color: root.idleBack
        radius: 5
        border.color: "#424242"
        anchors.fill: parent
    }

    Image {
        id: image
        width: 15
        height: 15
        anchors.verticalCenter: parent.verticalCenter
        source: "qrc:/qtquickplugin/images/template_image.png"
        anchors.horizontalCenter: parent.horizontalCenter
        fillMode: Image.PreserveAspectFit
    }
    states: [
        State {
            name: "idle"
            when: !root.hovered && !root.pressed && !root.checked
                  && root.enabled
        },
        State {
            name: "hover"
            when: root.hovered && !root.pressed && !root.checked && root.enabled
            PropertyChanges {
                target: bkg
                color: root.hoverBack
            }
        },
        State {
            name: "pressCheck"
            when: (root.pressed || root.checked) && root.enabled

            PropertyChanges {
                target: bkg
                color: root.hoverBack
                border.color: "#57b9fc"
            }
        },
        State {
            name: "blocked"
            when: !root.enabled

            PropertyChanges {
                target: bkg
                border.color: "#3a3a3a"
            }

            PropertyChanges {
                target: image
                opacity: 0.404
            }
        }
    ]
}
