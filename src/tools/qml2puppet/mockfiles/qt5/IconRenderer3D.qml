// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import QtQuick3D 1.15

Item {
    id: viewRoot
    width: 1024
    height: 1024
    visible: true

    property alias view3D: view3D
    property alias camPos: viewCamera.position

    function setSceneToBox()
    {
        selectionBox.targetNode = view3D.importScene;
    }

    function fitAndHideBox()
    {
        cameraControl.focusObject(selectionBox.model, viewCamera.eulerRotation, true, true);
        if (cameraControl._zoomFactor < 0.1)
            view3D.importScene.scale = view3D.importScene.scale.times(10);
        if (cameraControl._zoomFactor > 10)
            view3D.importScene.scale = view3D.importScene.scale.times(0.1);

        selectionBox.visible = false;
    }

    View3D {
        id: view3D
        camera: viewCamera
        environment: sceneEnv

        SceneEnvironment {
            id: sceneEnv
            antialiasingMode: SceneEnvironment.MSAA
            antialiasingQuality: SceneEnvironment.VeryHigh
        }

        PerspectiveCamera {
            id: viewCamera
            position: Qt.vector3d(-200, 200, 200)
            eulerRotation: Qt.vector3d(-45, -45, 0)
        }

        DirectionalLight {
            rotation: viewCamera.rotation
        }

        SelectionBox {
            id: selectionBox
            view3D: view3D
            geometryName: "SB"
        }

        EditCameraController {
            id: cameraControl
            camera: view3D.camera
            view3d: view3D
            ignoreToolState: true
        }
    }
}
