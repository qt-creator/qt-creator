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
                                     || centerMouseArea.dragging

    signal positionCommit()
    signal positionMove()

    Node {
        rotation: globalOrientation || !targetNode ? Qt.vector3d(0, 0, 0) : targetNode.sceneRotation

        Arrow {
            id: arrowX
            objectName: "Arrow X"
            rotation: Qt.vector3d(0, 0, -90)
            targetNode: moveGizmo.targetNode
            color: highlightOnHover && (hovering || dragging) ? Qt.lighter(Qt.rgba(1, 0, 0, 1))
                                                : Qt.rgba(1, 0, 0, 1)
            view3D: moveGizmo.view3D

            onPositionCommit: moveGizmo.positionCommit()
            onPositionMove: moveGizmo.positionMove()
        }

        Arrow {
            id: arrowY
            objectName: "Arrow Y"
            rotation: Qt.vector3d(0, 0, 0)
            targetNode: moveGizmo.targetNode
            color: highlightOnHover && (hovering || dragging) ? Qt.lighter(Qt.rgba(0, 0, 1, 1))
                                                : Qt.rgba(0, 0, 1, 1)
            view3D: moveGizmo.view3D

            onPositionCommit: moveGizmo.positionCommit()
            onPositionMove: moveGizmo.positionMove()
        }

        Arrow {
            id: arrowZ
            objectName: "Arrow Z"
            rotation: Qt.vector3d(90, 0, 0)
            targetNode: moveGizmo.targetNode
            color: highlightOnHover && (hovering || dragging) ? Qt.lighter(Qt.rgba(0, 0.6, 0, 1))
                                                : Qt.rgba(0, 0.6, 0, 1)
            view3D: moveGizmo.view3D

            onPositionCommit: moveGizmo.positionCommit()
            onPositionMove: moveGizmo.positionMove()
        }

    }

    Model {
        id: centerBall

        source: "#Sphere"
        scale: Qt.vector3d(0.024, 0.024, 0.024)
        materials: DefaultMaterial {
            id: material
            emissiveColor: highlightOnHover
                           && (centerMouseArea.hovering || centerMouseArea.dragging)
                           ? Qt.lighter(Qt.rgba(0.5, 0.5, 0.5, 1))
                           : Qt.rgba(0.5, 0.5, 0.5, 1)
            lighting: DefaultMaterial.NoLighting
        }

        MouseArea3D {
            id: centerMouseArea
            view3D: moveGizmo.view3D
            x: -60
            y: -60
            width: 120
            height: 120
            rotation: view3D.camera.rotation
            grabsMouse: moveGizmo.targetNode
            priority: 1

            property var _pointerPosPressed
            property var _targetStartPos

            function posInParent(pointerPosition)
            {
                var scenePointerPos = mapPositionToScene(pointerPosition);
                var sceneRelativeDistance = Qt.vector3d(
                            scenePointerPos.x - _pointerPosPressed.x,
                            scenePointerPos.y - _pointerPosPressed.y,
                            scenePointerPos.z - _pointerPosPressed.z);

                var newScenePos = Qt.vector3d(
                            _targetStartPos.x + sceneRelativeDistance.x,
                            _targetStartPos.y + sceneRelativeDistance.y,
                            _targetStartPos.z + sceneRelativeDistance.z);

                return moveGizmo.targetNode.parent.mapPositionFromScene(newScenePos);
            }

            onPressed: {
                if (!moveGizmo.targetNode)
                    return;

                _pointerPosPressed = mapPositionToScene(pointerPosition);
                var sp = moveGizmo.targetNode.scenePosition;
                _targetStartPos = Qt.vector3d(sp.x, sp.y, sp.z);
            }
            onDragged: {
                if (!moveGizmo.targetNode)
                    return;

                moveGizmo.targetNode.position = posInParent(pointerPosition);
                moveGizmo.positionMove();
            }
            onReleased: {
                if (!moveGizmo.targetNode)
                    return;

                moveGizmo.targetNode.position = posInParent(pointerPosition);
                moveGizmo.positionCommit();
            }
        }
    }
}
