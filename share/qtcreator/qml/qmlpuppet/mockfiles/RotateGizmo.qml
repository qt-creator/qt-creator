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
import QtQuick3D 1.15
import MouseArea3D 1.0

Node {
    id: rotateGizmo

    property View3D view3D
    property bool highlightOnHover: true
    property Node targetNode: null
    property bool globalOrientation: true
    readonly property bool dragging: cameraRing.dragging
                                     || rotRingX.dragging || rotRingY.dragging || rotRingZ.dragging
    property MouseArea3D dragHelper: null
    property real currentAngle
    property point currentMousePos
    property alias freeDraggerArea: mouseAreaFree

    position: dragHelper.pivotScenePosition(targetNode)

    onTargetNodeChanged: position = dragHelper.pivotScenePosition(targetNode)

    Connections {
        target: rotateGizmo.targetNode
        function onSceneTransformChanged()
        {
            rotateGizmo.position = rotateGizmo.dragHelper.pivotScenePosition(rotateGizmo.targetNode);
        }
    }

    signal rotateCommit()
    signal rotateChange()

    function copyRingProperties(srcRing) {
        draggingRing.rotation = srcRing.sceneRotation;
        draggingRing.color = srcRing.color;
        draggingRing.scale = srcRing.scale;
    }

    onDraggingChanged: {
        if (rotRingX.dragging)
            copyRingProperties(rotRingX)
        else if (rotRingY.dragging)
            copyRingProperties(rotRingY)
        else if (rotRingZ.dragging)
            copyRingProperties(rotRingZ)
    }

    Node {
        id: rotNode
        rotation: globalOrientation || !rotateGizmo.targetNode ? Qt.quaternion(1, 0, 0, 0)
                                                               : rotateGizmo.targetNode.sceneRotation
        visible: !rotateGizmo.dragging && !freeRotator.dragging

        RotateRing {
            id: rotRingX
            objectName: "Rotate Ring X"
            eulerRotation: Qt.vector3d(0, 90, 0)
            targetNode: rotateGizmo.targetNode
            color: highlightOnHover && (hovering || dragging) ? Qt.lighter(Qt.rgba(1, 0, 0, 1))
                                                              : Qt.rgba(1, 0, 0, 1)
            priority: 40
            view3D: rotateGizmo.view3D
            active: rotateGizmo.visible
            dragHelper: rotateGizmo.dragHelper

            onRotateCommit: rotateGizmo.rotateCommit()
            onRotateChange: rotateGizmo.rotateChange()
            onCurrentAngleChanged: rotateGizmo.currentAngle = currentAngle
            onCurrentMousePosChanged: rotateGizmo.currentMousePos = currentMousePos
        }

        RotateRing {
            id: rotRingY
            objectName: "Rotate Ring Y"
            eulerRotation: Qt.vector3d(90, 0, 0)
            targetNode: rotateGizmo.targetNode
            color: highlightOnHover && (hovering || dragging) ? Qt.lighter(Qt.rgba(0, 0.6, 0, 1))
                                                              : Qt.rgba(0, 0.6, 0, 1)
            // Just a smidge smaller than higher priority rings so that it doesn't obscure them
            scale: Qt.vector3d(0.998, 0.998, 0.998)
            priority: 30
            view3D: rotateGizmo.view3D
            active: rotateGizmo.visible
            dragHelper: rotateGizmo.dragHelper

            onRotateCommit: rotateGizmo.rotateCommit()
            onRotateChange: rotateGizmo.rotateChange()
            onCurrentAngleChanged: rotateGizmo.currentAngle = currentAngle
            onCurrentMousePosChanged: rotateGizmo.currentMousePos = currentMousePos
        }

        RotateRing {
            id: rotRingZ
            objectName: "Rotate Ring Z"
            eulerRotation: Qt.vector3d(0, 0, 0)
            targetNode: rotateGizmo.targetNode
            color: highlightOnHover && (hovering || dragging) ? Qt.lighter(Qt.rgba(0, 0, 1, 1))
                                                              : Qt.rgba(0, 0, 1, 1)
            // Just a smidge smaller than higher priority rings so that it doesn't obscure them
            scale: Qt.vector3d(0.996, 0.996, 0.996)
            priority: 20
            view3D: rotateGizmo.view3D
            active: rotateGizmo.visible
            dragHelper: rotateGizmo.dragHelper

            onRotateCommit: rotateGizmo.rotateCommit()
            onRotateChange: rotateGizmo.rotateChange()
            onCurrentAngleChanged: rotateGizmo.currentAngle = currentAngle
            onCurrentMousePosChanged: rotateGizmo.currentMousePos = currentMousePos
        }
    }

    RotateRing {
        // This ring is used as visual proxy during dragging to display the currently dragged
        // plane in static position, as rotation planes can wobble when ancestors don't have
        // uniform scaling.
        // Camera ring doesn't need dragging proxy as it doesn't wobble.
        id: draggingRing
        objectName: "draggingRing"
        targetNode: rotateGizmo.targetNode
        view3D: rotateGizmo.view3D
        active: false
        visible: rotRingX.dragging || rotRingY.dragging || rotRingZ.dragging
    }

    RotateRing {
        id: cameraRing
        objectName: "cameraRing"
        rotation: rotateGizmo.view3D.camera.rotation
        targetNode: rotateGizmo.targetNode
        color: highlightOnHover && (hovering || dragging) ? Qt.lighter(Qt.rgba(0.5, 0.5, 0.5, 1))
                                                          : Qt.rgba(0.5, 0.5, 0.5, 1)
        scale: Qt.vector3d(1.1, 1.1, 1.1)
        priority: 10
        view3D: rotateGizmo.view3D
        active: rotateGizmo.visible
        dragHelper: rotateGizmo.dragHelper
        visible: !rotRingX.dragging && !rotRingY.dragging && !rotRingZ.dragging && !freeRotator.dragging

        onRotateCommit: rotateGizmo.rotateCommit()
        onRotateChange: rotateGizmo.rotateChange()
        onCurrentAngleChanged: rotateGizmo.currentAngle = currentAngle
        onCurrentMousePosChanged: rotateGizmo.currentMousePos = currentMousePos
    }

    Model {
        id: freeRotator

        source: "#Sphere"
        materials: DefaultMaterial {
            id: material
            emissiveColor: "black"
            opacity: mouseAreaFree.hovering ? 0.15 : 0
            lighting: DefaultMaterial.NoLighting
        }
        scale: Qt.vector3d(0.15, 0.15, 0.15)
        visible: !rotateGizmo.dragging && !dragging

        property bool dragging: false
        property vector3d _pointerPosPressed
        property vector3d _targetPosOnScreen
        property vector3d _startRotation

        function handlePressed(screenPos)
        {
            if (!rotateGizmo.targetNode)
                return;

            // Need to recreate vector as we need to adjust it and we can't do that on reference of
            // scenePosition, which is read-only property
            var scenePos = rotateGizmo.dragHelper.pivotScenePosition(rotateGizmo.targetNode);
            _targetPosOnScreen = view3D.mapFrom3DScene(scenePos);
            _targetPosOnScreen.z = 0;
            _pointerPosPressed = Qt.vector3d(screenPos.x, screenPos.y, 0);

            // Recreate vector so we don't follow the changes in targetNode.rotation
            _startRotation = Qt.vector3d(rotateGizmo.targetNode.eulerRotation.x,
                                         rotateGizmo.targetNode.eulerRotation.y,
                                         rotateGizmo.targetNode.eulerRotation.z);
            dragging = true;
        }

        function handleDragged(screenPos)
        {
            if (!rotateGizmo.targetNode)
                return;

            mouseAreaFree.applyFreeRotation(
                        rotateGizmo.targetNode, _startRotation, _pointerPosPressed,
                        Qt.vector3d(screenPos.x, screenPos.y, 0), _targetPosOnScreen);

            rotateGizmo.rotateChange();
        }

        function handleReleased(screenPos)
        {
            if (!rotateGizmo.targetNode)
                return;

            mouseAreaFree.applyFreeRotation(
                        rotateGizmo.targetNode, _startRotation, _pointerPosPressed,
                        Qt.vector3d(screenPos.x, screenPos.y, 0), _targetPosOnScreen);

            rotateGizmo.rotateCommit();
            dragging = false;
        }

        MouseArea3D {
            id: mouseAreaFree
            view3D: rotateGizmo.view3D
            rotation: rotateGizmo.view3D.camera.rotation
            objectName: "Free rotator plane"
            x: -50
            y: -50
            width: 100
            height: 100
            circlePickArea: Qt.point(25, 50)
            grabsMouse: rotateGizmo.targetNode
            active: rotateGizmo.visible
            dragHelper: rotateGizmo.dragHelper

            onPressed: freeRotator.handlePressed(screenPos)
            onDragged: freeRotator.handleDragged(screenPos)
            onReleased: freeRotator.handleReleased(screenPos)
        }
    }
}
