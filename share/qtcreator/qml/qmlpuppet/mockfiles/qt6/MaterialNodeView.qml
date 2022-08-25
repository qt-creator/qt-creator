/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

import QtQuick3D 6.0

View3D {
    id: root
    anchors.fill: parent
    environment: sceneEnv
    camera: !envMode || (envMode === "SkyBox" && envValue === "preview_studio") ? studioCamera
                                                                                : defaultCamera

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
                                            : envMode === "Basic" ? SceneEnvironment.Transparent
                                                                  : SceneEnvironment.SkyBox
        clearColor: envMode === "Color" ? envValue : "#000000"
        lightProbe: !envMode || envMode === "SkyBox" ? skyBoxTex : null

        Texture {
            id: skyBoxTex
            source: !envMode ? "../images/preview_studio.hdr"
                             : envMode === "SkyBox" ? "../images/" + envValue + ".hdr"
                                                    : ""
        }
    }

    Node {
        DirectionalLight {
            eulerRotation.x: -26
            eulerRotation.y: modelSrc === "#Cube" ? -10 : -50
            brightness: envMode && envMode !== "SkyBox" ? 1 : 0
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
                    readonly property bool _edit3dLocked: true // Make this non-pickable
                    source: modelSrc ? modelSrc : "#Sphere"
                    eulerRotation.y: 45
                    materials: previewMaterial
                    scale: !modelSrc || modelSrc === "#Sphere"
                           ? Qt.vector3d(1.7, 1.7, 1.7) : Qt.vector3d(1.2, 1.2, 1.2)
                }
            }
        }
        Model {
            id: floorModel
            source: "#Rectangle"
            scale.y: 8
            scale.x: 8
            eulerRotation.x: -60
            visible: envMode === "Basic"
            materials: floorMaterial
            DefaultMaterial {
                id: floorMaterial
                diffuseMap: floorTex
                Texture {
                    id: floorTex
                    source: "../images/floor_tex.png"
                    scaleU: floorModel.scale.x
                    scaleV: floorModel.scale.y
                }
            }
        }
    }
}
