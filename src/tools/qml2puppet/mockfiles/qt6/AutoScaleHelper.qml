// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 6.0
import QtQuick3D 6.0
import MouseArea3D 1.0

Node {
    id: overlayNode

    property View3D view3D
    property Camera camera: view3D.camera
    property bool active: true

    // Read-only
    property real relativeScale: 1

    onActiveChanged: updateScale()
    onSceneTransformChanged: updateScale()
    // Trigger delayed update on camera change to ensure camera values are correct
    onCameraChanged: _generalHelper.requestOverlayUpdate();

    Connections {
        target: camera
        function onSceneTransformChanged() { updateScale() }
    }

    Connections {
        target: _generalHelper
        function onOverlayUpdateNeeded() { updateScale() }
    }

    function getScale(baseScale)
    {
        return Qt.vector3d(baseScale.x * relativeScale, baseScale.y * relativeScale,
                           baseScale.z * relativeScale);
    }

    function updateScale()
    {
        if (active)
            relativeScale = helper.getRelativeScale(overlayNode);
        else
            relativeScale = 1;
    }

    MouseArea3D {
        id: helper
        active: false
        view3D: overlayNode.view3D
    }
}
