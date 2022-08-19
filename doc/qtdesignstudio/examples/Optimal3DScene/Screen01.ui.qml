// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick 2.12
import QtQuick3D 1.15
import Optimal3DScene 1.0
import Quick3DAssets.High 1.0
import Quick3DAssets.Low 1.0
import Quick3DAssets.Merged 1.0
import QtQuick 2.15
import Quick3DAssets.VertexColor 1.0

Rectangle {
    width: Constants.width
    height: Constants.height

    color: Constants.backgroundColor
    property alias high: high

    View3D {
        id: view3D
        x: 0
        y: 0
        width: 1280
        height: 720
        SceneEnvironment {
            id: sceneEnvironment
            antialiasingQuality: SceneEnvironment.High
            antialiasingMode: SceneEnvironment.MSAA
        }

        Node {
            id: scenelow

            PerspectiveCamera {
                id: camera
                x: 0
                y: 501.999
                z: 953.07697
            }

            Low {
                id: low
                eulerRotation.z: 0.00001
                eulerRotation.y: -135
                eulerRotation.x: 0.00002
            }

            PointLight {
                id: lightPoint
                x: 0
                y: 888.433
                castsShadow: true
                brightness: 400
                quadraticFade: 0.01318
                z: -0.00007
            }
        }

        Node {
            id: scenehigh
            PerspectiveCamera {
                id: camera1
                x: 0
                y: 501.999
                z: 953.07697
            }

            PointLight {
                id: lightPoint1
                x: 0
                y: 888.433
                brightness: 400
                castsShadow: true
                z: -0.00007
                quadraticFade: 0.01318
            }

            High {
                id: high
                eulerRotation.y: -135
            }
        }

        Node {
            id: scenecombined
            PerspectiveCamera {
                id: camera2
                x: 0
                y: 501.999
                z: 953.07697
            }

            PointLight {
                id: lightPoint2
                x: 0
                y: 888.433
                brightness: 350
                castsShadow: true
                z: -0.00007
                quadraticFade: 0.01318
            }

            Merged {
                id: merged
                eulerRotation.y: -135
            }
        }

        Node {
            id: scenevertexcolor
            PerspectiveCamera {
                id: camera3
                x: 0
                y: 501.999
                z: 953.07697
            }

            PointLight {
                id: lightPoint3
                x: 0
                y: 888.433
                brightness: 350
                castsShadow: true
                z: -0.00007
                quadraticFade: 0.01318
            }

            VertexColor {
                id: vertexColor
                eulerRotation.y: -135
            }
        }
        environment: sceneEnvironment
    }
}

/*##^##
Designer {
    D{i:0;active3dScene:10;formeditorZoom:0.6600000262260437}D{i:3;invisible:true}D{i:7;invisible:true}
D{i:11;invisible:true}
}
##^##*/

