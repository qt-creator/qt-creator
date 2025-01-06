// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 6.0
import QtQuick3D 6.0

View3D {
    id: root
    anchors.fill: parent
    environment: sceneEnv
    camera: theCamera

    property Model sourceModel

    function fitToViewPort(closeUp)
    {
        // The magic number is the distance from camera default pos to origin
        _generalHelper.calculateNodeBoundsAndFocusCamera(theCamera, model, root,
                                                         1040, closeUp);
    }

    SceneEnvironment {
        id: sceneEnv
        antialiasingMode: SceneEnvironment.MSAA
        antialiasingQuality: SceneEnvironment.High
    }

    DirectionalLight {
        eulerRotation.x: -30
        eulerRotation.y: -30
    }

    PerspectiveCamera {
        id: theCamera
        z: 600
        y: 600
        x: 600
        eulerRotation.x: -45
        eulerRotation.y: -45
        clipFar: 10000
        clipNear: 1
    }

    Model {
        id: model
        source: _generalHelper.resolveAbsoluteSourceUrl(sourceModel)
        geometry: sourceModel.geometry
        materials: sourceModel.materials
    }
}
