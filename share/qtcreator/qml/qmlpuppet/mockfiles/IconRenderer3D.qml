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
        if (cameraControl._zoomFactor > 100)
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
        }
    }
}
