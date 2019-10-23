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

Model {
    id: arrow
    rotationOrder: Node.XYZr
    source: "meshes/Arrow.mesh"

    property View3D view3D
    property alias color: material.emissiveColor
    property Node targetNode: null

    readonly property bool hovering: mouseAreaYZ.hovering || mouseAreaXZ.hovering

    property var _pointerPosPressed
    property var _targetStartPos

    signal positionCommit()

    materials: DefaultMaterial {
        id: material
        emissiveColor: mouseAreaFront.hovering ? "white" : Qt.rgba(1.0, 0.0, 0.0, 1.0)
        lighting: DefaultMaterial.NoLighting
    }

    function handlePressed(mouseArea, pointerPosition)
    {
        if (!targetNode)
            return;

        var maskedPosition = Qt.vector3d(pointerPosition.x, 0, 0);
        _pointerPosPressed = mouseArea.mapPositionToScene(maskedPosition);
        var sp = targetNode.positionInScene;
        _targetStartPos = Qt.vector3d(sp.x, sp.y, sp.z);
    }

    function posInParent(mouseArea, pointerPosition)
    {
        var maskedPosition = Qt.vector3d(pointerPosition.x, 0, 0);
        var scenePointerPos = mouseArea.mapPositionToScene(maskedPosition);
        var sceneRelativeDistance = Qt.vector3d(
                    scenePointerPos.x - _pointerPosPressed.x,
                    scenePointerPos.y - _pointerPosPressed.y,
                    scenePointerPos.z - _pointerPosPressed.z);

        var newScenePos = Qt.vector3d(
                    _targetStartPos.x + sceneRelativeDistance.x,
                    _targetStartPos.y + sceneRelativeDistance.y,
                    _targetStartPos.z + sceneRelativeDistance.z);

        return targetNode.parent.mapPositionFromScene(newScenePos);
    }

    function handleDragged(mouseArea, pointerPosition)
    {
        if (!targetNode)
            return;

        targetNode.position = posInParent(mouseArea, pointerPosition);
    }

    function handleReleased(mouseArea, pointerPosition)
    {
        if (!targetNode)
            return;

        targetNode.position = posInParent(mouseArea, pointerPosition);
        arrow.positionCommit();
    }

    MouseArea3D {
        id: mouseAreaYZ
        view3D: arrow.view3D
        x: 0
        y: -1.5
        width: 12
        height: 3
        rotation: Qt.vector3d(0, 90, 0)
        grabsMouse: targetNode
        onPressed: arrow.handlePressed(mouseAreaYZ, pointerPosition)
        onDragged: arrow.handleDragged(mouseAreaYZ, pointerPosition)
        onReleased: arrow.handleReleased(mouseAreaYZ, pointerPosition)
    }

    MouseArea3D {
        id: mouseAreaXZ
        view3D: arrow.view3D
        x: 0
        y: -1.5
        width: 12
        height: 3
        rotation: Qt.vector3d(90, 90, 0)
        grabsMouse: targetNode
        onPressed: arrow.handlePressed(mouseAreaXZ, pointerPosition)
        onDragged: arrow.handleDragged(mouseAreaXZ, pointerPosition)
        onReleased: arrow.handleReleased(mouseAreaXZ, pointerPosition)
    }

}

