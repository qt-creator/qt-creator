// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Templates as T
import StudioTheme 1.0 as StudioTheme

Rectangle {
    id: control

    property StudioTheme.ControlStyle style: StudioTheme.Values.controlStyle

    property var pointA: Qt.vector2d()
    property var pointB: Qt.vector2d()

    property bool linked: false

    property var middle: {
        var ab = control.pointB.minus(control.pointA) // B - A
        return control.pointA.plus(ab.normalized().times(ab.length() * 0.5))
    }

    property var position: {
        // Calculate the middle point between A and B
        var ab = control.pointB.minus(control.pointA) // B - A
        var midAB = control.pointA.plus(ab.normalized().times(ab.length() * 0.5))
        var perpendicularAB = Qt.vector2d(ab.y, -ab.x)
        return midAB.plus(perpendicularAB.normalized().times(8.0 * StudioTheme.Values.scaleFactor))
    }

    color: "transparent"
    border.color: "transparent"

    x: control.position.x - (control.style.squareControlSize.width * 0.5)
    y: control.position.y - (control.style.squareControlSize.height * 0.5)

    implicitWidth: control.style.squareControlSize.width
    implicitHeight: control.style.squareControlSize.height

    transformOrigin: Item.Center

    T.Label {
        id: icon
        anchors.fill: parent
        text: control.linked ? StudioTheme.Constants.linked
                             : StudioTheme.Constants.unLinked
        visible: true
        color: "grey"
        font.family: StudioTheme.Constants.iconFont.family
        font.pixelSize: control.style.baseIconFontSize
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        anchors.margins: 4.0 * StudioTheme.Values.scaleFactor
        hoverEnabled: true
        onPressed: control.linked = !control.linked
    }

    states: [
        State {
            name: "default"
            when: !mouseArea.containsMouse
            PropertyChanges {
                target: icon
                color: "grey"//StudioTheme.Values.themeControlBackground
            }
        },
        State {
            name: "hover"
            when: mouseArea.containsMouse
            PropertyChanges {
                target: icon
                color: "white"//StudioTheme.Values.themeHoverHighlight
            }
        }
    ]
}
