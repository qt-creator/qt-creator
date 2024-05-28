// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick3D

View3D {
    id: sceneView
    anchors.fill: parent

    property bool usePerspective: false
    property alias showSceneLight: sceneLight.visible
    property alias showGrid: helperGrid.visible
    property alias gridColor: helperGrid.gridColor
    property alias perspectiveCamera: scenePerspectiveCamera
    property alias orthoCamera: sceneOrthoCamera
    property alias sceneEnv: sceneEnv
    property alias defaultLightProbe: defaultLightProbe
    property alias defaultCubeMap: defaultCubeMap
    property vector3d cameraLookAt
    property var selectionBoxes: []
    property Node selectedNode

    // Measuring the distance from camera to lookAt plus the distance of lookAt from grid plane
    // gives a reasonable grid spacing in most cases while keeping spacing constant when
    // orbiting the camera.
    readonly property double cameraDistance: {
        if (usePerspective) {
            // Round to five decimals to avoid rounding errors causing constant property updates
            // on the material when simply orbiting.
            let dist = cameraLookAt.minus(camera.position).length() + Math.abs(cameraLookAt.y)
            return Number(dist.toPrecision(5));
        }

        // Orthocamera should only care about camera magnification,
        // as grid will be same size regardless of distance, so setting steps based on distance
        // makes no sense.
        return 500 / orthoCamera.horizontalMagnification
    }

    camera: usePerspective ? scenePerspectiveCamera : sceneOrthoCamera

    environment: sceneEnv

    function ensureSelectionBoxes(count)
    {
        var needMore = count - selectionBoxes.length
        if (needMore > 0) {
            var component = Qt.createComponent("SelectionBox.qml");
            if (component.status === Component.Ready) {
                for (let i = 0; i < needMore; ++i) {
                    let geometryName = _generalHelper.generateUniqueName("SelectionBoxGeometry");
                    let box = component.createObject(sceneHelpers, {"view3D": sceneView,
                                                     "geometryName": geometryName});
                    selectionBoxes[selectionBoxes.length] = box;
                    box.showBox = Qt.binding(function() {return showSelectionBox;});
                }
            }
        }
    }

    SceneEnvironment {
        id: sceneEnv
        antialiasingMode: SceneEnvironment.MSAA
        antialiasingQuality: SceneEnvironment.High

        Texture {
            id: defaultLightProbe
        }

        CubeMapTexture {
            id: defaultCubeMap
        }
    }

    Node {
        id: sceneHelpers

        HelperGrid {
            id: helperGrid
            orthoMode: !sceneView.usePerspective
            distance: sceneView.cameraDistance
        }

        PointLight {
            id: sceneLight
            position: sceneView.usePerspective ? scenePerspectiveCamera.position
                                               : sceneOrthoCamera.position
            quadraticFade: 0
            linearFade: 0
        }

        ReflectionProbeBox {
            id: reflectionProbeBox
            selectedNode: sceneView.selectedNode
        }

        // Initial camera position and rotation should be such that they look at origin.
        // Otherwise EditCameraController._lookAtPoint needs to be initialized to correct
        // point.
        PerspectiveCamera {
            id: scenePerspectiveCamera
            z: 600
            y: 600
            eulerRotation.x: -45
            clipFar: 100000
            clipNear: 1
        }

        OrthographicCamera {
            id: sceneOrthoCamera
            z: 600
            y: 600
            eulerRotation.x: -45
            clipFar: 100000
            clipNear: -100000
        }
    }
}
