// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 6.0
import QtQuick3D 6.0
import MouseArea3D 1.0

Model {
    id: rotateRing

    property View3D view3D
    property alias color: material.diffuseColor
    property Node targetNode: null
    property bool dragging: mouseAreaMain.dragging
    property bool active: false
    property alias hovering: mouseAreaMain.hovering
    property alias priority: mouseAreaMain.priority
    property real currentAngle
    property point currentMousePos
    property MouseArea3D dragHelper: null

    property vector3d _pointerPosPressed
    property vector3d _targetPosOnScreen
    property vector3d _startRotation
    property bool _trackBall
    readonly property bool _edit3dLocked: true // Make this non-pickable

    signal rotateCommit()
    signal rotateChange()

    source: "../meshes/ring.mesh"

    Model {
        id: pickModel
        readonly property bool _edit3dLocked: true // Make this non-pickable in main picking handling
        objectName: "PickModel for " + rotateRing.objectName
        source: "../meshes/ringselect.mesh"
        pickable: true
    }

    materials: DefaultMaterial {
        id: material
        diffuseColor: "white"
        lighting: DefaultMaterial.NoLighting
    }

    function applyLocalRotation(screenPos)
    {
        currentAngle = mouseAreaMain.dragHelper.getNewRotationAngle(
                    targetNode, _pointerPosPressed, Qt.vector3d(screenPos.x, screenPos.y, 0),
                    _targetPosOnScreen, currentAngle, _trackBall);
        currentAngle = _generalHelper.adjustRotationForSnap(currentAngle);
        mouseAreaMain.dragHelper.applyRotationAngleToNode(targetNode, _startRotation, currentAngle);
    }

    function handlePressed(screenPos, angle)
    {
        if (!targetNode)
            return;

        if (targetNode == multiSelectionNode)
            _generalHelper.restartMultiSelection();

        // Need to recreate vector as we need to adjust it and we can't do that on reference of
        // scenePosition, which is read-only property
        var scenePos = mouseAreaMain.pivotScenePosition(targetNode);

        _targetPosOnScreen = view3D.mapFrom3DScene(scenePos);
        _targetPosOnScreen.z = 0;
        _pointerPosPressed = Qt.vector3d(screenPos.x, screenPos.y, 0);
        _trackBall = angle < 0.1;

        // Recreate vector so we don't follow the changes in targetNode.eulerRotation
        _startRotation = Qt.vector3d(targetNode.eulerRotation.x,
                                     targetNode.eulerRotation.y,
                                     targetNode.eulerRotation.z);
        // Ensure we never set NaN values for rotation, even if target node originally has them
        if (isNaN(_startRotation.x))
            _startRotation.x = 0;
        if (isNaN(_startRotation.y))
            _startRotation.y = 0;
        if (isNaN(_startRotation.z))
            _startRotation.z = 0;
        currentAngle = 0;
        currentMousePos = screenPos;
    }

    function handleDragged(screenPos)
    {
        if (!targetNode)
            return;

        applyLocalRotation(screenPos);

        if (targetNode == multiSelectionNode)
            _generalHelper.rotateMultiSelection(false);

        currentMousePos = screenPos;
        rotateChange();
    }

    function handleReleased(screenPos)
    {
        if (!targetNode)
            return;

        applyLocalRotation(screenPos);

        if (targetNode == multiSelectionNode)
            _generalHelper.rotateMultiSelection(true);

        rotateCommit();
        currentAngle = 0;
        currentMousePos = screenPos;

        if (targetNode == multiSelectionNode)
            _generalHelper.resetMultiSelectionNode();
    }

    MouseArea3D {
        id: mouseAreaMain
        view3D: rotateRing.view3D
        objectName: "Main plane of " + rotateRing.objectName
        x: -30
        y: -30
        width: 60
        height: 60
        circlePickArea: Qt.point(9.2, 1.4)
        grabsMouse: targetNode
        active: rotateRing.active
        pickNode: pickModel
        minAngle: 0.05
        dragHelper: rotateRing.dragHelper

        onPressed: (planePos, screenPos, angle)=> {
            rotateRing.handlePressed(screenPos, angle);
        }
        onDragged: (planePos, screenPos)=> {
            rotateRing.handleDragged(screenPos);
        }
        onReleased: (planePos, screenPos)=> {
            rotateRing.handleReleased(screenPos);
        }
    }
}
