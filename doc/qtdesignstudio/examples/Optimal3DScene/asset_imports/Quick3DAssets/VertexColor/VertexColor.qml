// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick 2.15
import QtQuick3D 1.15

Node {
    id: rootNode

    Model {
        id: roomBaseLow_005
        eulerRotation.x: -90
        scale.x: 800
        scale.y: 800
        scale.z: 800
        source: "meshes/roomBaseLow_005.mesh"

        DefaultMaterial {
            id: vert_material
            diffuseColor: "#ffa3a3a3"
        }
        materials: [
            vert_material
        ]
    }

    PointLight {
        id: point
        x: 2355.4
        y: -1009.92
        z: 2002.04
        eulerRotation.x: 90
        quadraticFade: 3.2e-07
    }
}

/*##^##
Designer {
    D{i:0;active3dScene:0}
}
##^##*/
