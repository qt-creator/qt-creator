// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 6.0
import QtQuick3D 6.0
import MouseArea3D 1.0

Model {
    id: rootModel

    property View3D view3D
    property alias color: material.diffuseColor
    property Node targetNode: null
    property bool dragging: mouseAreaYZ.dragging || mouseAreaXZ.dragging
    property bool active: false
    property MouseArea3D dragHelper: null
    property alias material: material
    property real length: 12
    property real offset: 0

    readonly property bool hovering: mouseAreaYZ.hovering || mouseAreaXZ.hovering
    readonly property bool _edit3dLocked: true // Make this non-pickable

    property vector3d _scenePosPressed
    property real _posPressed
    property vector3d _targetStartPos

    signal pressed(var mouseArea, point screenPos)
    signal dragged(var mouseArea, vector3d sceneRelativeDistance, real relativeDistance, point screenPos)
    signal released(var mouseArea, vector3d sceneRelativeDistance, real relativeDistance, point screenPos)

    DefaultMaterial {
        id: material
        diffuseColor: "white"
        lighting: DefaultMaterial.NoLighting
    }

    materials: [ material ]

    function handlePressed(mouseArea, planePos, screenPos)
    {
        if (!targetNode)
            return;

        var maskedPosition = Qt.vector3d(planePos.x, 0, 0);
        _posPressed = planePos.x;
        _scenePosPressed = mouseArea.dragHelper.mapPositionToScene(maskedPosition);
        _targetStartPos = mouseArea.pivotScenePosition(targetNode);
        pressed(mouseArea, screenPos);
    }

    function calcRelativeDistance(mouseArea, planePos)
    {
        var maskedPosition = Qt.vector3d(planePos.x, 0, 0);
        var scenePointerPos = mouseArea.dragHelper.mapPositionToScene(maskedPosition);
        return scenePointerPos.minus(_scenePosPressed);
    }

    function handleDragged(mouseArea, planePos, screenPos)
    {
        if (!targetNode)
            return;

        dragged(mouseArea, calcRelativeDistance(mouseArea, planePos), planePos.x - _posPressed, screenPos);
    }

    function handleReleased(mouseArea, planePos, screenPos)
    {
        if (!targetNode)
            return;

        released(mouseArea, calcRelativeDistance(mouseArea, planePos), planePos.x - _posPressed, screenPos);
    }

    MouseArea3D {
        id: mouseAreaYZ
        view3D: rootModel.view3D
        x: rootModel.offset
        y: -1.5
        width: rootModel.length
        height: 3
        eulerRotation: Qt.vector3d(0, 0, 90)
        grabsMouse: targetNode
        active: rootModel.active
        dragHelper: rootModel.dragHelper
        priority: 5

        onPressed: (planePos, screenPos)=> {
            rootModel.handlePressed(mouseAreaYZ, planePos, screenPos);
        }
        onDragged: (planePos, screenPos)=> {
            rootModel.handleDragged(mouseAreaYZ, planePos, screenPos);
        }
        onReleased: (planePos, screenPos)=> {
            rootModel.handleReleased(mouseAreaYZ, planePos, screenPos);
        }
    }

    MouseArea3D {
        id: mouseAreaXZ
        view3D: rootModel.view3D
        x: rootModel.offset
        y: -1.5
        width: rootModel.length
        height: 3
        eulerRotation: Qt.vector3d(0, 90, 90)
        grabsMouse: targetNode
        active: rootModel.active
        dragHelper: rootModel.dragHelper
        priority: 5

        onPressed: (planePos, screenPos)=> {
            rootModel.handlePressed(mouseAreaXZ, planePos, screenPos);
        }
        onDragged: (planePos, screenPos)=> {
            rootModel.handleDragged(mouseAreaXZ, planePos, screenPos);
        }
        onReleased: (planePos, screenPos)=> {
            rootModel.handleReleased(mouseAreaXZ, planePos, screenPos);
        }
    }
}

