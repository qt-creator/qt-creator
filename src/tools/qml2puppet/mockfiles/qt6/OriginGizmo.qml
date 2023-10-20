// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick3D

Item {
    id: root
    required property Node targetNode

    enum Axis {
        PositiveX,
        PositiveY,
        PositiveZ,
        NegativeX,
        NegativeY,
        NegativeZ
    }

    readonly property list<quaternion> rotations: [
        Qt.quaternion(Math.SQRT1_2, 0, Math.SQRT1_2, 0), // PositiveX
        Qt.quaternion(Math.SQRT1_2, -Math.SQRT1_2, 0, 0), // PositiveY
        Qt.quaternion(1, 0, 0, 0), // PositiveZ
        Qt.quaternion(Math.SQRT1_2, 0, -Math.SQRT1_2, 0), // NegativeX
        Qt.quaternion(Math.SQRT1_2, Math.SQRT1_2, 0, 0), // NegativeY
        Qt.quaternion(0, 0, 1, 0) // NegativeZ
    ]

    function quaternionForAxis(axis) {
        return rotations[axis]
    }

    signal axisClicked(int axis)

    QtObject {
        id: stylePalette
        property color brightBall: "#eeeeee"
        property color dimBall: "#111111"
        property color xAxis: "#ff0000"
        property color yAxis: "#00aa00"
        property color zAxis: "#1515ff"
        property color background: "#aa303030"
    }

    component LineRectangle : Rectangle {
        property vector2d startPoint: Qt.vector2d(0, 0)
        property vector2d endPoint: Qt.vector2d(0, 0)
        property real lineWidth: 5
        transformOrigin: Item.Left
        height: lineWidth
        antialiasing: true

        readonly property vector2d offset: startPoint.plus(endPoint).times(0.5);

        width: startPoint.minus(endPoint).length()
        rotation: Math.atan2(endPoint.y - startPoint.y, endPoint.x - startPoint.x) * 180 / Math.PI
    }

    Rectangle {
        id: ballBackground
        anchors.centerIn: parent
        width: parent.width > parent.height ? parent.height : parent.width
        height: width
        radius: width / 2
        color: ballBackgroundHoverHandler.hovered ? stylePalette.background : "transparent"
        antialiasing: true

        readonly property real subBallWidth: width / 5
        readonly property real subBallHalfWidth: subBallWidth * 0.5
        readonly property real subBallOffset: radius - subBallWidth / 2

        Item {
            anchors.centerIn: parent

            component SubBall : Rectangle {
                id: subBallRoot
                required property Node targetNode
                required property real offset

                property alias labelText: label.text
                property alias labelColor: label.color
                property alias labelVisible: label.visible
                property alias hovered: subBallHoverHandler.hovered
                property vector3d initialPosition: Qt.vector3d(0, 0, 0)
                readonly property vector3d position: quaternionVectorMultiply(targetNode.sceneRotation.inverted(), initialPosition)

                signal tapped()

                function quaternionVectorMultiply(q, v) {
                    let qv = Qt.vector3d(q.x, q.y, q.z)
                    let uv = qv.crossProduct(v)
                    let uuv = qv.crossProduct(uv)
                    uv = uv.times(2.0 * q.scalar)
                    uuv = uuv.times(2.0)
                    return v.plus(uv).plus(uuv)
                }

                antialiasing: true
                height: width
                radius: width / 2
                x: offset * position.x - width / 2
                y: offset * -position.y - height / 2
                z: position.z

                HoverHandler {
                    id: subBallHoverHandler
                }

                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.LeftButton
                    onClicked: (e) => {
                        subBallRoot.tapped()
                        e.accepted = true
                    }
                }

                Text {
                    id: label
                    anchors.centerIn: parent
                    antialiasing: true
                }
            }

            SubBall {
                id: positiveX
                targetNode: root.targetNode
                width: ballBackground.subBallWidth
                offset: ballBackground.subBallOffset
                labelText: qsTr("X")
                labelColor: hovered ? stylePalette.brightBall : stylePalette.dimBall
                color: stylePalette.xAxis
                initialPosition: Qt.vector3d(1, 0, 0)
                onTapped: {
                    let axis = OriginGizmo.Axis.PositiveX
                    if (_generalHelper.compareQuaternions(
                                root.targetNode.sceneRotation,
                                root.quaternionForAxis(OriginGizmo.Axis.PositiveX))) {
                        axis = OriginGizmo.Axis.NegativeX
                    }
                    root.axisClicked(axis)
                }
            }

            LineRectangle {
                endPoint: Qt.vector2d(positiveX.x + ballBackground.subBallHalfWidth, positiveX.y + ballBackground.subBallHalfWidth)
                color: stylePalette.xAxis
                z: positiveX.z - 0.001
            }

            SubBall {
                id: positiveY
                targetNode: root.targetNode
                width: ballBackground.subBallWidth
                offset: ballBackground.subBallOffset
                labelText: qsTr("Y")
                labelColor: hovered ? stylePalette.brightBall : stylePalette.dimBall
                color: stylePalette.yAxis
                initialPosition: Qt.vector3d(0, 1, 0)
                onTapped: {
                    let axis = OriginGizmo.Axis.PositiveY
                    if (_generalHelper.compareQuaternions(
                                root.targetNode.sceneRotation,
                                root.quaternionForAxis(OriginGizmo.Axis.PositiveY))) {
                        axis = OriginGizmo.Axis.NegativeY
                    }
                    root.axisClicked(axis)
                }
            }

            LineRectangle {
                endPoint: Qt.vector2d(positiveY.x + ballBackground.subBallHalfWidth, positiveY.y + ballBackground.subBallHalfWidth)
                color: stylePalette.yAxis
                z: positiveY.z - 0.001
            }

            SubBall {
                id: positiveZ
                targetNode: root.targetNode
                width: ballBackground.subBallWidth
                offset: ballBackground.subBallOffset
                labelText: qsTr("Z")
                labelColor: hovered ? stylePalette.brightBall : stylePalette.dimBall
                color: stylePalette.zAxis
                initialPosition: Qt.vector3d(0, 0, 1)
                onTapped: {
                    let axis = OriginGizmo.Axis.PositiveZ
                    if (_generalHelper.compareQuaternions(
                                root.targetNode.sceneRotation,
                                root.quaternionForAxis(OriginGizmo.Axis.PositiveZ))) {
                        axis = OriginGizmo.Axis.NegativeZ
                    }
                    root.axisClicked(axis)
                }
            }

            LineRectangle {
                endPoint: Qt.vector2d(positiveZ.x + ballBackground.subBallHalfWidth, positiveZ.y + ballBackground.subBallHalfWidth)
                color: stylePalette.zAxis
                z: positiveZ.z - 0.001
            }

            SubBall {
                targetNode: root.targetNode
                width: ballBackground.subBallWidth
                offset: ballBackground.subBallOffset
                labelText: qsTr("-X")
                labelColor: stylePalette.brightBall
                labelVisible: hovered
                color: Qt.rgba(stylePalette.xAxis.r, stylePalette.xAxis.g, stylePalette.xAxis.b, z + 1 * 0.5)
                border.color: stylePalette.xAxis
                border.width: 2
                initialPosition: Qt.vector3d(-1, 0, 0)
                onTapped: {
                    let axis = OriginGizmo.Axis.NegativeX
                    if (_generalHelper.compareQuaternions(
                                root.targetNode.sceneRotation,
                                root.quaternionForAxis(OriginGizmo.Axis.NegativeX))) {
                        axis = OriginGizmo.Axis.PositiveX
                    }
                    root.axisClicked(axis)
                }
            }

            SubBall {
                targetNode: root.targetNode
                width: ballBackground.subBallWidth
                offset: ballBackground.subBallOffset
                labelText: qsTr("-Y")
                labelColor: stylePalette.brightBall
                labelVisible: hovered
                color: Qt.rgba(stylePalette.yAxis.r, stylePalette.yAxis.g, stylePalette.yAxis.b, z + 1 * 0.5)
                border.color: stylePalette.yAxis
                border.width: 2
                initialPosition: Qt.vector3d(0, -1, 0)
                onTapped: {
                    let axis = OriginGizmo.Axis.NegativeY
                    if (_generalHelper.compareQuaternions(
                                root.targetNode.sceneRotation,
                                root.quaternionForAxis(OriginGizmo.Axis.NegativeY))) {
                        axis = OriginGizmo.Axis.PositiveY
                    }
                    root.axisClicked(axis)
                }
            }

            SubBall {
                targetNode: root.targetNode
                width: ballBackground.subBallWidth
                offset: ballBackground.subBallOffset
                labelText: qsTr("-Z")
                labelColor: stylePalette.brightBall
                labelVisible: hovered
                color: Qt.rgba(stylePalette.zAxis.r, stylePalette.zAxis.g, stylePalette.zAxis.b, z + 1 * 0.5)
                border.color: stylePalette.zAxis
                border.width: 2
                initialPosition: Qt.vector3d(0, 0, -1)
                onTapped: {
                    let axis = OriginGizmo.Axis.NegativeZ
                    if (_generalHelper.compareQuaternions(
                                root.targetNode.sceneRotation,
                                root.quaternionForAxis(OriginGizmo.Axis.NegativeZ))) {
                        axis = OriginGizmo.Axis.PositiveZ
                    }
                    root.axisClicked(axis)
                }
            }
        }

        HoverHandler {
            id: ballBackgroundHoverHandler
            acceptedDevices: PointerDevice.Mouse
            cursorShape: Qt.PointingHandCursor
        }
    }
}
