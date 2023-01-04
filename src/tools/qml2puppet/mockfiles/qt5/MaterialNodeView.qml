// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick3D 1.15

View3D {
    id: root
    anchors.fill: parent
    environment: sceneEnv
    camera: envMode === "SkyBox" && envValue === "preview_studio" ? studioCamera : defaultCamera

    property Material previewMaterial
    property string envMode
    property string envValue
    property string modelSrc: "#Sphere"

    function fitToViewPort(closeUp)
    {
        // No need to zoom this view, this is here just to avoid runtime warnings
    }

    SceneEnvironment {
        id: sceneEnv
        antialiasingMode: SceneEnvironment.MSAA
        antialiasingQuality: SceneEnvironment.High
        backgroundMode: envMode === "Color" ? SceneEnvironment.Color
                                            : envMode === "SkyBox" ? SceneEnvironment.SkyBox
                                                                   : SceneEnvironment.Transparent
        clearColor: envMode === "Color" ? envValue : "#000000"
        lightProbe: envMode === "SkyBox" ? skyBoxTex : null

        Texture {
            id: skyBoxTex
            source: envMode === "SkyBox" ? "../images/" + envValue + ".hdr"
                                         : ""
        }
    }

    Node {
        DirectionalLight {
            eulerRotation.x: -26
            eulerRotation.y: modelSrc === "#Cube" ? -10 : -50
            brightness: envMode !== "SkyBox" ? 100 : 0
        }

        PerspectiveCamera {
            id: defaultCamera
            y: 70
            z: 200
            eulerRotation.x: -5.71
            clipNear: 1
            clipFar: 1000
        }

        PerspectiveCamera {
            id: studioCamera
            y: 232
            z: 85
            eulerRotation.x: -64.98
            clipNear: 1
            clipFar: 1000
        }

        Node {
            rotation: root.camera.rotation
            y: 50
            Node {
                y: modelSrc === "#Cone" ? -40 : 10
                eulerRotation.x: 35
                Model {
                    id: model
                    source: modelSrc ? modelSrc : "#Sphere"
                    eulerRotation.y: 45
                    materials: previewMaterial
                    scale: !modelSrc || modelSrc === "#Sphere"
                           ? Qt.vector3d(1.7, 1.7, 1.7) : Qt.vector3d(1.2, 1.2, 1.2)
                }
            }
        }
    }
}
