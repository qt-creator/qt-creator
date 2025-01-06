// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 6.0
import QtQuick3D 6.0
import CameraGeometry 1.0

Model {
    id: cameraFrustum

    property alias geometryName: cameraGeometry.name // Name must be unique for each geometry
    property alias viewPortRect: cameraGeometry.viewPortRect
    property Node targetNode: null
    property Node scene: null
    property bool selected: false
    readonly property bool _edit3dLocked: true // Make this non-pickable

    function updateGeometry()
    {
        cameraGeometry.update();
    }

    position: targetNode ? targetNode.scenePosition : Qt.vector3d(0, 0, 0)
    rotation: targetNode ? targetNode.sceneRotation : Qt.quaternion(1, 0, 0, 0)

    geometry: cameraGeometry
    materials: [
        DefaultMaterial {
            id: defaultMaterial
            diffuseColor: cameraFrustum.selected ? "#FF0000" : "#555555"
            lighting: DefaultMaterial.NoLighting
            cullMode: Material.NoCulling
        }
    ]

    CameraGeometry {
        id: cameraGeometry
        camera: cameraFrustum.scene && cameraFrustum.targetNode ? cameraFrustum.targetNode : null
    }
}
