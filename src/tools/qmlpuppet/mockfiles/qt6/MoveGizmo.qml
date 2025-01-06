// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 6.0
import QtQuick3D 6.0
import MouseArea3D 1.0

Node {
    id: moveGizmo

    property View3D view3D
    property bool highlightOnHover: false
    property Node targetNode: null
    property bool globalOrientation: true
    readonly property bool dragging: arrowX.dragging || arrowY.dragging || arrowZ.dragging
                                     || planeX.dragging || planeY.dragging || planeZ.dragging
                                     || centerBall.dragging
    property MouseArea3D dragHelper: null
    property alias freeDraggerArea: centerBall.mouseArea

    position: dragHelper.pivotScenePosition(targetNode)

    onTargetNodeChanged: position = dragHelper.pivotScenePosition(targetNode)

    Connections {
        target: moveGizmo.targetNode
        function onSceneTransformChanged()
        {
            moveGizmo.position = moveGizmo.dragHelper.pivotScenePosition(moveGizmo.targetNode);
        }
    }

    signal positionCommit()
    signal positionMove()

    Node {
        rotation: globalOrientation || !moveGizmo.targetNode ? Qt.quaternion(1, 0, 0, 0)
                                                             : moveGizmo.targetNode.sceneRotation
        Arrow {
            id: arrowX
            eulerRotation: Qt.vector3d(0, 0, -90)
            targetNode: moveGizmo.targetNode
            color: highlightOnHover && (hovering || dragging) ? Qt.lighter(Qt.rgba(1, 0, 0, 1))
                                                              : Qt.rgba(1, 0, 0, 1)
            view3D: moveGizmo.view3D
            active: moveGizmo.visible
            dragHelper: moveGizmo.dragHelper

            dragAxis: Qt.vector3d(1, 0, 0)
            globalOrientation: moveGizmo.globalOrientation

            onPositionCommit: moveGizmo.positionCommit()
            onPositionMove: moveGizmo.positionMove()
        }

        Arrow {
            id: arrowY
            eulerRotation: Qt.vector3d(0, 0, 0)
            targetNode: moveGizmo.targetNode
            color: highlightOnHover && (hovering || dragging) ? Qt.lighter(Qt.rgba(0, 0.6, 0, 1))
                                                              : Qt.rgba(0, 0.6, 0, 1)
            view3D: moveGizmo.view3D
            active: moveGizmo.visible
            dragHelper: moveGizmo.dragHelper

            dragAxis: Qt.vector3d(0, 1, 0)
            globalOrientation: moveGizmo.globalOrientation

            onPositionCommit: moveGizmo.positionCommit()
            onPositionMove: moveGizmo.positionMove()
        }

        Arrow {
            id: arrowZ
            eulerRotation: Qt.vector3d(90, 0, 0)
            targetNode: moveGizmo.targetNode
            color: highlightOnHover && (hovering || dragging) ? Qt.lighter(Qt.rgba(0, 0, 1, 1))
                                                              : Qt.rgba(0, 0, 1, 1)
            view3D: moveGizmo.view3D
            active: moveGizmo.visible
            dragHelper: moveGizmo.dragHelper

            dragAxis: Qt.vector3d(0, 0, 1)
            globalOrientation: moveGizmo.globalOrientation

            onPositionCommit: moveGizmo.positionCommit()
            onPositionMove: moveGizmo.positionMove()
        }

        PlanarMoveHandle {
            id: planeX

            y: 10
            z: 10

            eulerRotation: Qt.vector3d(0, 90, 0)
            targetNode: moveGizmo.targetNode
            color: highlightOnHover && (hovering || dragging) ? Qt.lighter(Qt.rgba(1, 0, 0, 1))
                                                              : Qt.rgba(1, 0, 0, 1)
            view3D: moveGizmo.view3D
            active: moveGizmo.visible
            dragHelper: moveGizmo.dragHelper

            dragAxes: Qt.vector3d(0, 1, 1)
            globalOrientation: moveGizmo.globalOrientation

            onPositionCommit: moveGizmo.positionCommit()
            onPositionMove: moveGizmo.positionMove()
        }

        PlanarMoveHandle {
            id: planeY

            x: 10
            z: 10

            eulerRotation: Qt.vector3d(90, 0, 0)
            targetNode: moveGizmo.targetNode
            color: highlightOnHover && (hovering || dragging) ? Qt.lighter(Qt.rgba(0, 0.6, 0, 1))
                                                              : Qt.rgba(0, 0.6, 0, 1)
            view3D: moveGizmo.view3D
            active: moveGizmo.visible
            dragHelper: moveGizmo.dragHelper

            dragAxes: Qt.vector3d(1, 0, 1)
            globalOrientation: moveGizmo.globalOrientation

            onPositionCommit: moveGizmo.positionCommit()
            onPositionMove: moveGizmo.positionMove()
        }

        PlanarMoveHandle {
            id: planeZ

            x: 10
            y: 10

            eulerRotation: Qt.vector3d(0, 0, 0)
            targetNode: moveGizmo.targetNode
            color: highlightOnHover && (hovering || dragging) ? Qt.lighter(Qt.rgba(0, 0, 1, 1))
                                                              : Qt.rgba(0, 0, 1, 1)
            view3D: moveGizmo.view3D
            active: moveGizmo.visible
            dragHelper: moveGizmo.dragHelper

            dragAxes: Qt.vector3d(1, 1, 0)
            globalOrientation: moveGizmo.globalOrientation

            onPositionCommit: moveGizmo.positionCommit()
            onPositionMove: moveGizmo.positionMove()
        }
    }

    PlanarMoveHandle {
        id: centerBall

        source: "#Sphere"
        color: highlightOnHover && (hovering || dragging) ? Qt.lighter(Qt.rgba(0.5, 0.5, 0.5, 1))
                                                          : Qt.rgba(0.5, 0.5, 0.5, 1)
        rotation: view3D.camera.rotation
        priority: 10
        targetNode: moveGizmo.targetNode

        view3D: moveGizmo.view3D
        active: moveGizmo.visible
        dragHelper: moveGizmo.dragHelper

        dragAxes: Qt.vector3d(1, 1, 1)
        globalOrientation: moveGizmo.globalOrientation

        onPositionCommit: moveGizmo.positionCommit()
        onPositionMove: moveGizmo.positionMove()
    }
}
