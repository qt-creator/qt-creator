// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 6.0
import QtQuick3D 6.0

Item {
    id: root
    property Node targetNode
    property View3D targetView

    property vector3d offset: Qt.vector3d(0, 0, 0)

    property bool isBehindCamera

    onTargetNodeChanged: updateOverlay()

    Connections {
        target: targetNode
        function onSceneTransformChanged() { updateOverlay() }
    }

    Connections {
        target: targetView.camera
        function onSceneTransformChanged() { updateOverlay() }
    }

    Connections {
        target: _generalHelper
        function onOverlayUpdateNeeded() { updateOverlay() }
    }

    function updateOverlay()
    {
        var scenePos = targetNode ? targetNode.scenePosition : Qt.vector3d(0, 0, 0);
        // Need separate variable as scenePos is reference to read-only property
        var scenePosWithOffset = Qt.vector3d(scenePos.x + offset.x,
                                             scenePos.y + offset.y,
                                             scenePos.z + offset.z);
        var viewPos = targetView ? targetView.mapFrom3DScene(scenePosWithOffset)
                                 : Qt.vector3d(0, 0, 0);
        root.x = viewPos.x;
        root.y = viewPos.y;

        isBehindCamera = viewPos.z <= 0;
    }
}
