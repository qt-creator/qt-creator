// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick 2.15
import QtQuick3D 1.15

Node {
    id: rootNode

    Model {
        id: ovenLow
        x: -600
        z: -300
        eulerRotation.x: -90
        scale.x: 100
        scale.y: 100
        scale.z: 100
        source: "meshes/ovenLow.mesh"

        DefaultMaterial {
            id: oven_material
            diffuseColor: "#ffa3a3a3"
        }

        DefaultMaterial {
            id: induction_material
            diffuseColor: "#ff040404"
        }
        materials: [
            oven_material,
            induction_material
        ]
    }

    Model {
        id: tapLow
        x: -679.799
        y: 420
        z: 100
        eulerRotation.x: -90
        scale.x: 100
        scale.y: 100
        scale.z: 100
        source: "meshes/tapLow.mesh"
        materials: [
            oven_material
        ]
    }

    Model {
        id: plateLow_002
        y: 424.176
        z: 600
        eulerRotation.x: -90
        eulerRotation.y: -19.7049
        scale.x: 100
        scale.y: 100
        scale.z: 100
        source: "meshes/plateLow_002.mesh"

        DefaultMaterial {
            id: plate_material
            diffuseColor: "#ff8fa365"
        }
        materials: [
            plate_material
        ]
    }

    Model {
        id: plateLow_001
        x: -200
        y: 417.734
        z: 600
        eulerRotation.x: -90
        scale.x: 100
        scale.y: 100
        scale.z: 100
        source: "meshes/plateLow_001.mesh"
        materials: [
            plate_material
        ]
    }

    Model {
        id: plateLow
        y: 417.734
        z: 600
        eulerRotation.x: -90
        scale.x: 100
        scale.y: 100
        scale.z: 100
        source: "meshes/plateLow.mesh"
        materials: [
            plate_material
        ]
    }

    Model {
        id: fridgeLow
        x: 300
        y: 0.82016
        z: 600
        eulerRotation.x: -90
        scale.x: 100
        scale.y: 100
        scale.z: 100
        source: "meshes/fridgeLow.mesh"
        materials: [
            oven_material
        ]
    }

    Model {
        id: roofLightLow
        x: -0.000179373
        y: 1200.82
        z: -1.67638e-06
        eulerRotation.x: -90
        scale.x: 100
        scale.y: 100
        scale.z: 100
        source: "meshes/roofLightLow.mesh"

        DefaultMaterial {
            id: lamp_material
            diffuseColor: "#ff0c0c0c"
        }
        materials: [
            lamp_material
        ]
    }

    Model {
        id: kitchenTilesLow
        x: 200
        y: 430
        z: 700
        eulerRotation.x: -90
        scale.x: 100
        scale.y: 100
        scale.z: 100
        source: "meshes/kitchenTilesLow.mesh"

        DefaultMaterial {
            id: tiles_material
            diffuseColor: "#ffcccccc"
        }
        materials: [
            tiles_material
        ]
    }

    Model {
        id: sinkCabinLow
        x: -600
        z: 300
        eulerRotation.x: -90
        scale.x: 100
        scale.y: 100
        scale.z: 100
        source: "meshes/sinkCabinLow.mesh"

        DefaultMaterial {
            id: sink_material
            diffuseColor: "#ff2d2d2d"
        }

        DefaultMaterial {
            id: cabinDoor_material
            diffuseColor: "#ff505050"
        }
        materials: [
            sink_material,
            oven_material,
            cabinDoor_material
        ]
    }

    Model {
        id: corner
        x: -600
        z: 600
        eulerRotation.x: -90
        scale.x: 100
        scale.y: 100
        scale.z: 100
        source: "meshes/corner.mesh"

        DefaultMaterial {
            id: cabinTop_material
            diffuseColor: "#ff121212"
        }
        materials: [
            cabinTop_material,
            oven_material
        ]
    }

    Model {
        id: roomBaseLow
        eulerRotation.x: -90
        scale.x: 800
        scale.y: 800
        scale.z: 800
        source: "meshes/roomBaseLow.mesh"

        DefaultMaterial {
            id: floor_material
            diffuseColor: "#ff353b2a"
        }

        DefaultMaterial {
            id: wall_material
            diffuseColor: "#ff7f85a3"
        }

        DefaultMaterial {
            id: roof_material
            diffuseColor: "#ff747474"
        }
        materials: [
            floor_material,
            wall_material,
            roof_material
        ]
    }

    Model {
        id: cabinsLeftLow
        x: -100
        y: 238.4
        z: 553.885
        eulerRotation.x: -90
        scale.x: 100
        scale.y: 100
        scale.z: 100
        source: "meshes/cabinsLeftLow.mesh"
        materials: [
            oven_material,
            cabinDoor_material,
            cabinTop_material
        ]
    }
}

/*##^##
Designer {
    D{i:0;active3dScene:0}
}
##^##*/
