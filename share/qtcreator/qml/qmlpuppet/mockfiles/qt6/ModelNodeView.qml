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

import QtQuick 6.0
import QtQuick3D 6.0

View3D {
    id: root
    anchors.fill: parent
    environment: sceneEnv
    camera: theCamera

    property bool ready: false
    property real prevZoomFactor: -1
    property Model sourceModel

    function fitToViewPort()
    {
        cameraControl.focusObject(model, theCamera.eulerRotation, true, false);

        if (cameraControl._zoomFactor < 0.1) {
            model.scale = model.scale.times(10);
        } else if (cameraControl._zoomFactor > 10) {
            model.scale = model.scale.times(0.1);
        } else {
            // We need one more render after zoom factor change, so only set ready when zoom factor
            // or scaling hasn't changed from the previous frame
            ready = _generalHelper.fuzzyCompare(cameraControl._zoomFactor, prevZoomFactor);
            prevZoomFactor = cameraControl._zoomFactor;
        }
    }

    SceneEnvironment {
        id: sceneEnv
        antialiasingMode: SceneEnvironment.MSAA
        antialiasingQuality: SceneEnvironment.High
    }

    EditCameraController {
        id: cameraControl
        camera: theCamera
        anchors.fill: parent
        view3d: root
        ignoreToolState: true
    }

    DirectionalLight {
        eulerRotation.x: -30
        eulerRotation.y: -30
    }

    PerspectiveCamera {
        id: theCamera
        z: 600
        y: 600
        eulerRotation.x: -45
        clipFar: 10000
        clipNear: 1
    }

    Model {
        id: model
        readonly property bool _edit3dLocked: true // Make this non-pickable
        eulerRotation.y: 45

        source: sourceModel.source
        geometry: sourceModel.geometry

        materials: [
            DefaultMaterial {
                diffuseColor: "#4aee45"
            }
        ]
    }
}
