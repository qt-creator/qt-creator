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
    id: scaleGizmo

    property View3D view3D
    property bool highlightOnHover: false
    property Node targetNode: null
    property bool globalOrientation: true
    readonly property bool dragging: scaleRodX.dragging || scaleRodY.dragging || scaleRodZ.dragging
                                     || planeX.dragging || planeY.dragging || planeZ.dragging
                                     || centerMouseArea.dragging
    property MouseArea3D dragHelper: null

    position: targetNode ? targetNode.scenePosition : Qt.vector3d(0, 0, 0)
    orientation: targetNode ? targetNode.orientation : Node.LeftHanded

    signal scaleCommit()
    signal scaleChange()

    Node {
        rotation: globalOrientation || !targetNode ? Qt.vector3d(0, 0, 0) : targetNode.sceneRotation
        rotationOrder: scaleGizmo.targetNode ? scaleGizmo.targetNode.rotationOrder : Node.YXZ
        orientation: scaleGizmo.orientation

        ScaleRod {
            id: scaleRodX
            rotation: Qt.vector3d(0, 0, -90)
            targetNode: scaleGizmo.targetNode
            color: highlightOnHover && (hovering || dragging) ? Qt.lighter(Qt.rgba(1, 0, 0, 1))
                                                              : Qt.rgba(1, 0, 0, 1)
            view3D: scaleGizmo.view3D
            active: scaleGizmo.visible
            globalOrientation: scaleGizmo.globalOrientation
            dragHelper: scaleGizmo.dragHelper

            onScaleCommit: scaleGizmo.scaleCommit()
            onScaleChange: scaleGizmo.scaleChange()
        }

        ScaleRod {
            id: scaleRodY
            rotation: Qt.vector3d(0, 0, 0)
            targetNode: scaleGizmo.targetNode
            color: highlightOnHover && (hovering || dragging) ? Qt.lighter(Qt.rgba(0, 0.6, 0, 1))
                                                              : Qt.rgba(0, 0.6, 0, 1)
            view3D: scaleGizmo.view3D
            active: scaleGizmo.visible
            globalOrientation: scaleGizmo.globalOrientation
            dragHelper: scaleGizmo.dragHelper

            onScaleCommit: scaleGizmo.scaleCommit()
            onScaleChange: scaleGizmo.scaleChange()
        }

        ScaleRod {
            id: scaleRodZ
            rotation: Qt.vector3d(90, 0, 0)
            targetNode: scaleGizmo.targetNode
            color: highlightOnHover && (hovering || dragging) ? Qt.lighter(Qt.rgba(0, 0, 1, 1))
                                                              : Qt.rgba(0, 0, 1, 1)
            view3D: scaleGizmo.view3D
            active: scaleGizmo.visible
            globalOrientation: scaleGizmo.globalOrientation
            dragHelper: scaleGizmo.dragHelper

            onScaleCommit: scaleGizmo.scaleCommit()
            onScaleChange: scaleGizmo.scaleChange()
        }

        PlanarScaleHandle {
            id: planeX

            y: 10
            z: 10

            rotation: Qt.vector3d(0, 90, 0)
            targetNode: scaleGizmo.targetNode
            color: highlightOnHover && (hovering || dragging) ? Qt.lighter(Qt.rgba(1, 0, 0, 1))
                                                              : Qt.rgba(1, 0, 0, 1)
            view3D: scaleGizmo.view3D
            active: scaleGizmo.visible
            globalOrientation: scaleGizmo.globalOrientation
            dragHelper: scaleGizmo.dragHelper

            onScaleCommit: scaleGizmo.scaleCommit()
            onScaleChange: scaleGizmo.scaleChange()
        }

        PlanarScaleHandle {
            id: planeY

            x: 10
            z: 10

            rotation: Qt.vector3d(90, 0, 0)
            targetNode: scaleGizmo.targetNode
            color: highlightOnHover && (hovering || dragging) ? Qt.lighter(Qt.rgba(0, 0.6, 0, 1))
                                                              : Qt.rgba(0, 0.6, 0, 1)
            view3D: scaleGizmo.view3D
            active: scaleGizmo.visible
            globalOrientation: scaleGizmo.globalOrientation
            dragHelper: scaleGizmo.dragHelper

            onScaleCommit: scaleGizmo.scaleCommit()
            onScaleChange: scaleGizmo.scaleChange()
        }

        PlanarScaleHandle {
            id: planeZ

            x: 10
            y: 10

            rotation: Qt.vector3d(0, 0, 0)
            targetNode: scaleGizmo.targetNode
            color: highlightOnHover && (hovering || dragging) ? Qt.lighter(Qt.rgba(0, 0, 1, 1))
                                                              : Qt.rgba(0, 0, 1, 1)
            view3D: scaleGizmo.view3D
            active: scaleGizmo.visible
            globalOrientation: scaleGizmo.globalOrientation
            dragHelper: scaleGizmo.dragHelper

            onScaleCommit: scaleGizmo.scaleCommit()
            onScaleChange: scaleGizmo.scaleChange()
        }
    }

    Model {
        id: centerCube

        source: "#Cube"
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
            view3D: scaleGizmo.view3D
            x: -60
            y: -60
            width: 120
            height: 120
            rotation: view3D.camera.rotation
            grabsMouse: scaleGizmo.targetNode
            priority: 1
            active: scaleGizmo.visible
            dragHelper: scaleGizmo.dragHelper

            property vector3d _startScale
            property point _startScreenPos

            function localScale(screenPos)
            {
                var yDelta = screenPos.y - _startScreenPos.y;
                if (yDelta === 0)
                    return _startScale;
                var scaler = 1.0 + (yDelta * 0.025);
                if (scaler === 0)
                    scaler = 0.0001;
                if (scaler < 0)
                    scaler = -scaler;
                return Qt.vector3d(scaler * _startScale.x,
                                   scaler * _startScale.y,
                                   scaler * _startScale.z);
            }

            onPressed: {
                if (!scaleGizmo.targetNode)
                    return;

                // Recreate vector so we don't follow the changes in targetNode.scale
                _startScale = Qt.vector3d(scaleGizmo.targetNode.scale.x,
                                          scaleGizmo.targetNode.scale.y,
                                          scaleGizmo.targetNode.scale.z);
                _startScreenPos = screenPos;
            }
            onDragged: {
                if (!scaleGizmo.targetNode)
                    return;
                scaleGizmo.targetNode.scale = localScale(screenPos);
                scaleGizmo.scaleChange();
            }
            onReleased: {
                if (!scaleGizmo.targetNode)
                    return;

                scaleGizmo.targetNode.scale = localScale(screenPos);
                scaleGizmo.scaleCommit();
            }
        }
    }
}
