// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Shapes
import QtQuick.Templates as T
import StudioTheme 1.0 as StudioTheme

Rectangle {
    id: control

    property StudioTheme.ControlStyle style: StudioTheme.Values.controlStyle

    property Item __parentControl

    property bool linked: linkXZ.linked || linkYZ.linked || linkXY.linked

    color: "transparent"
    border.color: "transparent"

    implicitWidth: control.style.squareControlSize.width
    implicitHeight: control.style.squareControlSize.height

    z: 10

    function linkAll() {
        linkXY.linked = linkYZ.linked = linkXZ.linked = true
    }

    function unlinkAll() {
        linkXY.linked = linkYZ.linked = linkXZ.linked = false
    }

    T.Label {
        id: linkIndicatorIcon
        anchors.fill: parent
        text: control.linked ? StudioTheme.Constants.linked
                             : StudioTheme.Constants.unLinked
        visible: true
        color: control.style.indicator.idle
        font.family: StudioTheme.Constants.iconFont.family
        font.pixelSize: control.style.baseIconFontSize
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        onPressed: linkPopup.opened ? linkPopup.close() : linkPopup.open()
    }

    T.Popup {
        id: linkPopup

        x: 50
        y: 0

        // TODO proper size
        width: 20 + (3 * control.style.squareControlSize.width)
        height: 20 + (3 * control.style.squareControlSize.height)

        padding: control.style.borderWidth
        margins: 0 // If not defined margin will be -1

        closePolicy: T.Popup.CloseOnPressOutside | T.Popup.CloseOnPressOutsideParent
                     | T.Popup.CloseOnEscape | T.Popup.CloseOnReleaseOutside
                     | T.Popup.CloseOnReleaseOutsideParent

        contentItem: Item {
            id: content
            anchors.fill: parent

            Rectangle {
                width: triangle.diameter
                height: triangle.diameter
                anchors.centerIn: parent
                color: "transparent"

                Shape {
                    // Kept for debugging purposes
                    id: triangle

                    visible: false

                    property real diameter: 30 * StudioTheme.Values.scaleFactor
                    property real radius: triangle.diameter * 0.5

                    width: triangle.diameter
                    height: triangle.diameter

                    ShapePath {
                        id: path

                        property real height: triangle.diameter * Math.cos(Math.PI / 6)

                        property vector2d pX: Qt.vector2d(triangle.radius, 0)
                        property vector2d pY: Qt.vector2d(0, path.height)
                        property vector2d pZ: Qt.vector2d(triangle.diameter, path.height)

                        property vector2d center: Qt.vector2d(triangle.radius, triangle.radius)

                        strokeWidth: control.style.borderWidth
                        strokeColor: triangleMouseArea.containsMouse ? "white" : "gray"
                        strokeStyle: ShapePath.SolidLine
                        fillColor: "transparent"

                        startX: path.pX.x
                        startY: path.pX.y

                        PathLine { x: path.pY.x; y: path.pY.y }
                        PathLine { x: path.pZ.x; y: path.pZ.y }
                        PathLine { x: path.pX.x; y: path.pX.y }
                    }
                }

                Item {
                    id: triangleMouseArea

                    anchors.fill: parent

                    property alias mouseX: tmpMouseArea.mouseX
                    property alias mouseY: tmpMouseArea.mouseY

                    signal clicked

                    function sign(p1, p2, p3) {
                        return (p1.x - p3.x) * (p2.y - p3.y) - (p2.x - p3.x) * (p1.y - p3.y)
                    }

                    function pointInTriangle(pt, v1, v2, v3) {
                        var d1 = sign(pt, v1, v2)
                        var d2 = sign(pt, v2, v3)
                        var d3 = sign(pt, v3, v1)

                        var has_neg = (d1 < 0) || (d2 < 0) || (d3 < 0)
                        var has_pos = (d1 > 0) || (d2 > 0) || (d3 > 0)

                        return !(has_neg && has_pos)
                    }

                    property bool containsMouse: {
                        if (!tmpMouseArea.containsMouse)
                            return false

                        var point = Qt.vector2d(triangleMouseArea.mouseX, triangleMouseArea.mouseY)
                        return pointInTriangle(point, path.pX, path.pZ, path.pY)
                    }


                    onClicked: {
                        if (linkXZ.linked && linkYZ.linked && linkXY.linked)
                            control.unlinkAll()
                        else
                            control.linkAll()
                    }

                    MouseArea {
                        id: tmpMouseArea
                        anchors.fill: parent
                        hoverEnabled: true
                        acceptedButtons: Qt.LeftButton | Qt.RightButton
                        onClicked: {
                            if (triangleMouseArea.containsMouse)
                                triangleMouseArea.clicked()
                        }
                    }
                }

                // https://stackoverflow.com/questions/38164074/how-to-create-a-round-mouse-area-in-qml
                // https://stackoverflow.com/questions/2049582/how-to-determine-if-a-point-is-in-a-2d-triangle

                T.Label {
                    id: triangleIcon
                    anchors.fill: parent
                    anchors.bottomMargin: 3 * StudioTheme.Values.scaleFactor
                    text: StudioTheme.Constants.linkTriangle
                    visible: true
                    color: triangleMouseArea.containsMouse ? "white" : "gray"
                    font.family: StudioTheme.Constants.iconFont.family
                    font.pixelSize: control.style.baseIconFontSize * 3
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignHCenter
                }

                LinkIndicator3DComponent {
                    id: linkXZ
                    style: control.style
                    pointA: path.pX
                    pointB: path.pZ
                    rotation: 105 // 60
                }

                LinkIndicator3DComponent {
                    id: linkYZ
                    style: control.style
                    pointA: path.pZ
                    pointB: path.pY
                    rotation: 45 // -180
                }

                LinkIndicator3DComponent {
                    id: linkXY
                    style: control.style
                    pointA: path.pY
                    pointB: path.pX
                    rotation: -15 // -60
                }

                T.Label {
                    id: xIcon
                    x: path.pX.x - (xIcon.width * 0.5)
                    y: path.pX.y - xIcon.height
                    text: "X"
                    color: StudioTheme.Values.theme3DAxisXColor

                    font.family: StudioTheme.Constants.font.family
                    font.pixelSize: control.style.baseIconFontSize
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignHCenter
                }
                T.Label {
                    id: yIcon
                    x: path.pY.x - yIcon.width - (4.0 * StudioTheme.Values.scaleFactor)
                    y: path.pY.y - (yIcon.height * 0.5)
                    text: "Y"
                    color: StudioTheme.Values.theme3DAxisYColor

                    font.family: StudioTheme.Constants.font.family
                    font.pixelSize: control.style.baseIconFontSize
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignHCenter
                }
                T.Label {
                    id: zIcon
                    x: path.pZ.x + (4.0 * StudioTheme.Values.scaleFactor)
                    y: path.pZ.y - (zIcon.height * 0.5)
                    text: "Z"
                    color: StudioTheme.Values.theme3DAxisZColor

                    font.family: StudioTheme.Constants.font.family
                    font.pixelSize: control.style.baseIconFontSize
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignHCenter
                }

            }
        }

        background: Rectangle {
            color: control.style.background.idle
            border.color: control.style.interaction
            border.width: control.style.borderWidth
        }

        enter: Transition {}
        exit: Transition {}
    }

    states: [
        State {
            name: "default"
            when: !mouseArea.containsMouse && !linkPopup.opened
            PropertyChanges {
                target: linkIndicatorIcon
                color: control.style.indicator.idle
            }
        },
        State {
            name: "hover"
            when: mouseArea.containsMouse && !linkPopup.opened
            PropertyChanges {
                target: linkIndicatorIcon
                color: control.style.indicator.hover
            }
        },
        State {
            name: "active"
            when: linkPopup.opened
            PropertyChanges {
                target: linkIndicatorIcon
                color: control.style.indicator.interaction
            }
        }
    ]
}
