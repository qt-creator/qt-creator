// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 6.0
import QtQuick3D 6.0
import MouseArea3D 1.0

DirectionalDraggable {
    id: scaleRod
    source: "../meshes/scalerod.mesh"

    property vector3d axis

    signal scaleCommit()
    signal scaleChange()

    property vector3d _startScale

    Model {
        readonly property bool _edit3dLocked: true // Make this non-pickable
        source: "#Cube"
        y: 10
        scale: Qt.vector3d(0.020, 0.020, 0.020)
        materials: DefaultMaterial {
            id: material
            diffuseColor: scaleRod.color
            lighting: DefaultMaterial.NoLighting
        }
    }

    onPressed: {
        if (targetNode == multiSelectionNode)
            _generalHelper.restartMultiSelection();
        _startScale = targetNode.scale;
    }

    onDragged: (mouseArea, sceneRelativeDistance, relativeDistance)=> {
        targetNode.scale = mouseArea.getNewScale(_startScale, Qt.vector2d(relativeDistance, 0),
                                                 axis, Qt.vector3d(0, 0, 0));
        if (targetNode == multiSelectionNode)
            _generalHelper.scaleMultiSelection(false);
        scaleChange();
    }

    onReleased: (mouseArea, sceneRelativeDistance, relativeDistance)=> {
        targetNode.scale = mouseArea.getNewScale(_startScale, Qt.vector2d(relativeDistance, 0),
                                                 axis, Qt.vector3d(0, 0, 0));
        if (targetNode == multiSelectionNode)
            _generalHelper.scaleMultiSelection(true);
        scaleCommit();
    }
}
