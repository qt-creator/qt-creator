// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 6.0
import QtQuick3D 6.0

View3D {
    id: sceneView
    anchors.fill: parent

    property bool usePerspective: false
    property alias showSceneLight: sceneLight.visible
    property alias showGrid: helperGrid.visible
    property alias gridColor: helperGrid.gridColor
    property alias sceneHelpers: sceneHelpers
    property alias perspectiveCamera: scenePerspectiveCamera
    property alias orthoCamera: sceneOrthoCamera
    property alias sceneEnv: sceneEnv
    property vector3d cameraLookAt

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
    SceneEnvironment {
        id: sceneEnv
        antialiasingMode: SceneEnvironment.MSAA
        antialiasingQuality: SceneEnvironment.High
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
