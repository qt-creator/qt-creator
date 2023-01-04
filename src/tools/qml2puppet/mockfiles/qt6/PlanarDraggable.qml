// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 6.0
import QtQuick3D 6.0
import MouseArea3D 1.0

Model {
    id: rootModel

    property View3D view3D
    property alias color: gizmoMaterial.diffuseColor
    property alias priority: mouseArea.priority
    property Node targetNode: null
    property bool dragging: mouseArea.dragging
    property bool active: false
    property MouseArea3D dragHelper: null
    property alias mouseArea: mouseArea

    readonly property bool hovering: mouseArea.hovering
    readonly property bool _edit3dLocked: true // Make this non-pickable

    property vector3d _scenePosPressed
    property vector2d _planePosPressed
    property vector3d _targetStartPos

    signal pressed(var mouseArea)
    signal dragged(var mouseArea, vector3d sceneRelativeDistance, vector2d relativeDistance)
    signal released(var mouseArea, vector3d sceneRelativeDistance, vector2d relativeDistance)

    source: "#Rectangle"

    DefaultMaterial {
        id: gizmoMaterial
        diffuseColor: "white"
        lighting: DefaultMaterial.NoLighting
        cullMode: Material.NoCulling
    }
    materials: gizmoMaterial

    function handlePressed(mouseArea, planePos)
    {
        if (!targetNode)
            return;

        _planePosPressed = planePos;
        _scenePosPressed = mouseArea.dragHelper.mapPositionToScene(planePos.toVector3d());
        _targetStartPos = mouseArea.pivotScenePosition(targetNode);
        pressed(mouseArea);
    }

    function calcRelativeDistance(mouseArea, planePos)
    {
        var scenePointerPos = mouseArea.dragHelper.mapPositionToScene(planePos.toVector3d());
        return scenePointerPos.minus(_scenePosPressed);
    }

    function handleDragged(mouseArea, planePos)
    {
        if (!targetNode)
            return;

        dragged(mouseArea, calcRelativeDistance(mouseArea, planePos),
                planePos.minus(_planePosPressed));
    }

    function handleReleased(mouseArea, planePos)
    {
        if (!targetNode)
            return;

        released(mouseArea, calcRelativeDistance(mouseArea, planePos),
                 planePos.minus(_planePosPressed));
    }

    MouseArea3D {
        id: mouseArea
        view3D: rootModel.view3D
        x: -60
        y: -60
        width: 120
        height: 120
        grabsMouse: targetNode
        active: rootModel.active
        dragHelper: rootModel.dragHelper

        onPressed: (planePos)=> {
            rootModel.handlePressed(mouseArea, planePos);
        }
        onDragged: (planePos)=> {
            rootModel.handleDragged(mouseArea, planePos);
        }
        onReleased: (planePos)=> {
            rootModel.handleReleased(mouseArea, planePos);
        }
    }
}

