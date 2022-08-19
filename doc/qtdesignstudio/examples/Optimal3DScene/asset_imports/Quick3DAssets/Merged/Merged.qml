// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick 2.15
import QtQuick3D 1.15

Node {
    id: rootNode

    Model {
        id: roomBaseLow_002
        eulerRotation.x: -90
        scale.x: 800
        scale.y: 800
        scale.z: 800
        source: "meshes/roomBaseLow_002.mesh"

        DefaultMaterial {
            id: colormap_material
            diffuseMap: element
            diffuseColor: "#ffffff"

            Texture {
                id: element
                source: "colormap.png"
            }
        }
        materials: [
            colormap_material
        ]
    }
}

/*##^##
Designer {
    D{i:0;active3dScene:0}
}
##^##*/
