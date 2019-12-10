/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

import QtQuick 2.0
import QtQuick3D 1.0
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

    position: targetNode ? targetNode.scenePosition : Qt.vector3d(0, 0, 0)
    orientation: targetNode ? targetNode.orientation : Node.LeftHanded

    signal positionCommit()
    signal positionMove()

    Node {
        rotation: globalOrientation || !moveGizmo.targetNode ? Qt.vector3d(0, 0, 0)
                                                             : moveGizmo.targetNode.sceneRotation
        rotationOrder: moveGizmo.targetNode ? moveGizmo.targetNode.rotationOrder : Node.YXZ
        orientation: moveGizmo.orientation

        Arrow {
            id: arrowX
            rotation: Qt.vector3d(0, 0, -90)
            targetNode: moveGizmo.targetNode
            color: highlightOnHover && (hovering || dragging) ? Qt.lighter(Qt.rgba(1, 0, 0, 1))
                                                              : Qt.rgba(1, 0, 0, 1)
            view3D: moveGizmo.view3D
            active: moveGizmo.visible
            dragHelper: moveGizmo.dragHelper

            onPositionCommit: moveGizmo.positionCommit()
            onPositionMove: moveGizmo.positionMove()
        }

        Arrow {
            id: arrowY
            rotation: Qt.vector3d(0, 0, 0)
            targetNode: moveGizmo.targetNode
            color: highlightOnHover && (hovering || dragging) ? Qt.lighter(Qt.rgba(0, 0.6, 0, 1))
                                                              : Qt.rgba(0, 0.6, 0, 1)
            view3D: moveGizmo.view3D
            active: moveGizmo.visible
            dragHelper: moveGizmo.dragHelper

            onPositionCommit: moveGizmo.positionCommit()
            onPositionMove: moveGizmo.positionMove()
        }

        Arrow {
            id: arrowZ
            rotation: Qt.vector3d(90, 0, 0)
            targetNode: moveGizmo.targetNode
            color: highlightOnHover && (hovering || dragging) ? Qt.lighter(Qt.rgba(0, 0, 1, 1))
                                                              : Qt.rgba(0, 0, 1, 1)
            view3D: moveGizmo.view3D
            active: moveGizmo.visible
            dragHelper: moveGizmo.dragHelper

            onPositionCommit: moveGizmo.positionCommit()
            onPositionMove: moveGizmo.positionMove()
        }

        PlanarMoveHandle {
            id: planeX

            y: 10
            z: 10

            rotation: Qt.vector3d(0, 90, 0)
            targetNode: moveGizmo.targetNode
            color: highlightOnHover && (hovering || dragging) ? Qt.lighter(Qt.rgba(1, 0, 0, 1))
                                                              : Qt.rgba(1, 0, 0, 1)
            view3D: moveGizmo.view3D
            active: moveGizmo.visible
            dragHelper: moveGizmo.dragHelper

            onPositionCommit: moveGizmo.positionCommit()
            onPositionMove: moveGizmo.positionMove()
        }

        PlanarMoveHandle {
            id: planeY

            x: 10
            z: 10

            rotation: Qt.vector3d(90, 0, 0)
            targetNode: moveGizmo.targetNode
            color: highlightOnHover && (hovering || dragging) ? Qt.lighter(Qt.rgba(0, 0.6, 0, 1))
                                                              : Qt.rgba(0, 0.6, 0, 1)
            view3D: moveGizmo.view3D
            active: moveGizmo.visible
            dragHelper: moveGizmo.dragHelper

            onPositionCommit: moveGizmo.positionCommit()
            onPositionMove: moveGizmo.positionMove()
        }

        PlanarMoveHandle {
            id: planeZ

            x: 10
            y: 10

            rotation: Qt.vector3d(0, 0, 0)
            targetNode: moveGizmo.targetNode
            color: highlightOnHover && (hovering || dragging) ? Qt.lighter(Qt.rgba(0, 0, 1, 1))
                                                              : Qt.rgba(0, 0, 1, 1)
            view3D: moveGizmo.view3D
            active: moveGizmo.visible
            dragHelper: moveGizmo.dragHelper

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
        priority: 1
        targetNode: moveGizmo.targetNode

        view3D: moveGizmo.view3D
        active: moveGizmo.visible
        dragHelper: moveGizmo.dragHelper

        onPositionCommit: moveGizmo.positionCommit()
        onPositionMove: moveGizmo.positionMove()
    }
}
