// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 6.0
import QtQuick3D 6.0
import MouseArea3D 1.0

PlanarDraggable {
    id: planarHandle
    scale: Qt.vector3d(0.024, 0.024, 0.024)

    property vector3d dragAxes
    property bool globalOrientation

    signal positionCommit()
    signal positionMove()

    function localPos(sceneRelativeDistance)
    {
        var newScenePos = Qt.vector3d(
                    _targetStartPos.x + sceneRelativeDistance.x,
                    _targetStartPos.y + sceneRelativeDistance.y,
                    _targetStartPos.z + sceneRelativeDistance.z);
        newScenePos = _generalHelper.adjustTranslationForSnap(newScenePos, _targetStartPos,
                                                              dragAxes, globalOrientation,
                                                              targetNode);
        return targetNode.parent ? targetNode.parent.mapPositionFromScene(newScenePos) : newScenePos;
    }

    onPressed: {
        if (targetNode == multiSelectionNode)
            _generalHelper.restartMultiSelection();
    }

    onDragged: (mouseArea, sceneRelativeDistance)=> {
        targetNode.position = localPos(sceneRelativeDistance);
        if (targetNode == multiSelectionNode)
            _generalHelper.moveMultiSelection(false);
        positionMove();
    }

    onReleased: (mouseArea, sceneRelativeDistance)=> {
        targetNode.position = localPos(sceneRelativeDistance);
        if (targetNode == multiSelectionNode)
            _generalHelper.moveMultiSelection(true);
        positionCommit();
    }
}
