// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 6.0
import QtQuick3D 6.0
import MouseArea3D 1.0

PlanarDraggable {
    id: planarHandle
    scale: Qt.vector3d(0.024, 0.024, 0.024)

    property bool globalOrientation: false
    property vector3d axisX
    property vector3d axisY

    signal scaleCommit()
    signal scaleChange()

    property vector3d _startScale

    onPressed: {
        if (targetNode == multiSelectionNode)
            _generalHelper.restartMultiSelection();
        _startScale = targetNode.scale;
    }

    onDragged: (mouseArea, sceneRelativeDistance, relativeDistance)=> {
        targetNode.scale = mouseArea.getNewScale(_startScale, relativeDistance.times(scale.x),
                                                 axisX, axisY);
        if (targetNode == multiSelectionNode)
            _generalHelper.scaleMultiSelection(false);
        scaleChange();
    }

    onReleased: (mouseArea, sceneRelativeDistance, relativeDistance)=> {
        targetNode.scale = mouseArea.getNewScale(_startScale, relativeDistance.times(scale.x),
                                                 axisX, axisY);
        if (targetNode == multiSelectionNode)
            _generalHelper.scaleMultiSelection(true);
        scaleCommit();
    }
}
