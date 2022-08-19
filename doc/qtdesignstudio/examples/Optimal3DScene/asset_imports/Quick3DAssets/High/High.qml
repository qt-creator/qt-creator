// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick 2.15
import QtQuick3D 1.15

Node {
    id: rootNode

    Model {
        id: floor
        eulerRotation.x: -90
        scale.x: 800
        scale.y: 800
        scale.z: 800
        source: "meshes/floor.mesh"

        DefaultMaterial {
            id: floor_material
            diffuseColor: "#ff353b2a"
        }
        materials: [
            floor_material
        ]
    }

    Model {
        id: ovenLatch
        x: -500
        y: 100
        z: -350
        eulerRotation.x: -44.54277
        scale.x: 100
        scale.y: 100
        scale.z: 100
        source: "meshes/ovenLatch.mesh"
        eulerRotation.z: -90
        eulerRotation.y: 90

        DefaultMaterial {
            id: induction_material
            diffuseColor: "#ff040404"
        }

        DefaultMaterial {
            id: oven_material
            diffuseColor: "#ffa3a3a3"
        }
        materials: [
            induction_material,
            oven_material
        ]
    }

    Model {
        id: ovenHigh
        x: -600
        z: -300
        eulerRotation.x: -90
        scale.x: 100
        scale.y: 100
        scale.z: 100
        source: "meshes/ovenHigh.mesh"
        materials: [
            oven_material,
            induction_material
        ]
    }

    Model {
        id: tapHigh
        x: -679.799
        y: 420
        z: 100
        eulerRotation.x: -90
        scale.x: 100
        scale.y: 100
        scale.z: 100
        source: "meshes/tapHigh.mesh"
        materials: [
            oven_material
        ]
    }

    Model {
        id: fridgeDoor
        x: 499
        y: 450.82
        z: 500
        eulerRotation.x: -89.98022
        scale.x: 100
        scale.y: 100
        scale.z: 100
        source: "meshes/fridgeDoor.mesh"
        eulerRotation.z: -160.70996
        eulerRotation.y: 17.65012
        materials: [
            oven_material
        ]
    }

    Model {
        id: fridgeHigh
        x: 300
        y: 0.82016
        z: 600
        eulerRotation.x: -90
        scale.x: 100
        scale.y: 100
        scale.z: 100
        source: "meshes/fridgeHigh.mesh"
        materials: [
            oven_material
        ]
    }

    Model {
        id: plateHigh
        y: 417.734
        z: 600
        eulerRotation.x: -90
        scale.x: 100
        scale.y: 100
        scale.z: 100
        source: "meshes/plateHigh.mesh"

        DefaultMaterial {
            id: plate_material
            diffuseColor: "#ff8fa365"
        }
        materials: [
            plate_material
        ]
    }

    Model {
        id: plateHigh_001
        x: -200
        y: 417.734
        z: 600
        eulerRotation.x: -90
        scale.x: 100
        scale.y: 100
        scale.z: 100
        source: "meshes/plateHigh_001.mesh"
        materials: [
            plate_material
        ]
    }

    Model {
        id: plateHigh_002
        y: 424.176
        z: 600
        eulerRotation.x: -90
        eulerRotation.y: -19.7049
        scale.x: 100
        scale.y: 100
        scale.z: 100
        source: "meshes/plateHigh_002.mesh"
        materials: [
            plate_material
        ]
    }

    Model {
        id: roofLightHigh
        x: -0.000179373
        y: 1200.82
        z: -1.67638e-06
        eulerRotation.x: -90
        scale.x: 100
        scale.y: 100
        scale.z: 100
        source: "meshes/roofLightHigh.mesh"

        DefaultMaterial {
            id: lamp_material
            diffuseColor: "#ff0c0c0c"
        }
        materials: [
            lamp_material
        ]
    }

    Model {
        id: roofHighpoly
        y: 1200
        z: -0.999999
        eulerRotation.x: -90
        scale.x: 800
        scale.y: 800
        scale.z: 800
        source: "meshes/roofHighpoly.mesh"

        DefaultMaterial {
            id: roof_material
            diffuseColor: "#ff747474"
        }
        materials: [
            roof_material
        ]
    }

    Model {
        id: sinkCabinHigh_002
        x: -500
        y: 250
        z: -99
        eulerRotation.x: -89.97202
        scale.x: 100
        scale.y: 100
        scale.z: 100
        source: "meshes/sinkCabinHigh_002.mesh"
        eulerRotation.z: -18.43495
        eulerRotation.y: 45

        DefaultMaterial {
            id: cabinDoor_material
            diffuseColor: "#ff505050"
        }
        materials: [
            cabinDoor_material
        ]
    }

    Model {
        id: sinkCabinHigh_001
        x: -500
        y: 250
        z: 299
        eulerRotation.x: -90
        scale.x: 100
        scale.y: 100
        scale.z: 100
        source: "meshes/sinkCabinHigh_001.mesh"
        eulerRotation.z: 0
        eulerRotation.y: 81.49729
        materials: [
            cabinDoor_material
        ]
    }

    Node {
        id: tileHolderHigh
        x: 200
        y: 430
        z: 700
        eulerRotation.x: -90
        scale.x: 100
        scale.y: 100
        scale.z: 100

        Model {
            id: kitchenTile_005
            x: -9
            y: 0.299997
            z: 9.53674e-07
            eulerRotation.z: -90
            source: "meshes/kitchenTile_005.mesh"

            DefaultMaterial {
                id: tiles_material
                diffuseColor: "#f6f4f4"
            }
            materials: [
                tiles_material
            ]
        }

        Model {
            id: kitchenTile_004
            x: -9
            y: 0.299997
            z: 1.1
            eulerRotation.z: -90
            source: "meshes/kitchenTile_004.mesh"
            materials: [
                tiles_material
            ]
        }

        Model {
            id: kitchenTile_002
            x: -9
            y: 0.299997
            z: 2.2
            eulerRotation.z: -90
            source: "meshes/kitchenTile_002.mesh"
            materials: [
                tiles_material
            ]
        }

        Model {
            id: kitchenTile_003
            y: -9.53674e-07
            z: 2.2
            source: "meshes/kitchenTile_003.mesh"
            materials: [
                tiles_material
            ]
        }

        Model {
            id: kitchenTile_001
            y: -9.53674e-07
            z: 1.1
            source: "meshes/kitchenTile_001.mesh"
            materials: [
                tiles_material
            ]
        }

        Model {
            id: kitchenTile
            source: "meshes/kitchenTile.mesh"
            materials: [
                tiles_material
            ]
        }
    }

    Model {
        id: sinkCabinHigh
        x: -600
        z: 300
        eulerRotation.x: -90
        scale.x: 100
        scale.y: 100
        scale.z: 100
        source: "meshes/sinkCabinHigh.mesh"

        DefaultMaterial {
            id: sink_material
            diffuseColor: "#ff2d2d2d"
        }
        materials: [
            sink_material,
            oven_material
        ]
    }

    Model {
        id: cornerHigh
        x: -600
        z: 600
        eulerRotation.x: -90
        scale.x: 100
        scale.y: 100
        scale.z: 100
        source: "meshes/cornerHigh.mesh"

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
        id: cabinsLeftDoor3
        x: -399
        y: 250
        z: 500
        scale.x: 100
        scale.y: 100
        scale.z: 100
        source: "meshes/cabinsLeftDoor3.mesh"
        eulerRotation.x: -89.05581
        eulerRotation.z: 179.99979
        eulerRotation.y: -135.19951
        materials: [
            cabinDoor_material
        ]
    }

    Model {
        id: cabinsLeftDoor2
        x: -199
        y: 250
        z: 500
        eulerRotation.x: -89.98022
        scale.x: 100
        scale.y: 100
        scale.z: 100
        source: "meshes/cabinsLeftDoor2.mesh"
        eulerRotation.z: 53.1301
        eulerRotation.y: -21.80141
        materials: [
            cabinDoor_material
        ]
    }

    Model {
        id: cabinsLeftDoor1
        x: 0.999999
        y: 250
        z: 500
        scale.x: 100
        scale.y: 100
        scale.z: 100
        source: "meshes/cabinsLeftDoor1.mesh"
        eulerRotation.x: -88.52515
        eulerRotation.z: -108.53853
        eulerRotation.y: 173.47327
        materials: [
            cabinDoor_material
        ]
    }

    Model {
        id: cabinsLeftHigh
        x: -100
        y: 240.447
        z: 580.588
        eulerRotation.x: -90
        scale.x: 100
        scale.y: 100
        scale.z: 100
        source: "meshes/cabinsLeftHigh.mesh"
        materials: [
            oven_material,
            cabinTop_material
        ]
    }

    Model {
        id: roomBaseHigh
        eulerRotation.x: -90
        scale.x: 800
        scale.y: 800
        scale.z: 800
        source: "meshes/roomBaseHigh.mesh"

        DefaultMaterial {
            id: wall_material
            diffuseColor: "#ff7f85a3"
        }
        materials: [
            wall_material
        ]
    }
}


