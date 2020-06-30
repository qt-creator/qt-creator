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

Model {
    id: rootModel

    property View3D view3D
    property alias color: gizmoMaterial.emissiveColor
    property alias priority: mouseArea.priority
    property Node targetNode: null
    property bool dragging: mouseArea.dragging
    property bool active: false
    property MouseArea3D dragHelper: null
    property alias mouseArea: mouseArea

    readonly property bool hovering: mouseArea.hovering

    property vector3d _scenePosPressed
    property vector2d _planePosPressed
    property vector3d _targetStartPos

    signal pressed(var mouseArea)
    signal dragged(var mouseArea, vector3d sceneRelativeDistance, vector2d relativeDistance)
    signal released(var mouseArea, vector3d sceneRelativeDistance, vector2d relativeDistance)

    source: "#Rectangle"

    DefaultMaterial {
        id: gizmoMaterial
        emissiveColor: "white"
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

        onPressed: rootModel.handlePressed(mouseArea, planePos)
        onDragged: rootModel.handleDragged(mouseArea, planePos)
        onReleased: rootModel.handleReleased(mouseArea, planePos)
    }
}

