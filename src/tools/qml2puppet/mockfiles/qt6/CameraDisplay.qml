// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
import QtQuick
import QtQuick3D

Item {
    id: cameraDisplay

    required property Camera camera
    required property size preferredSize
    required property size viewPortSize
    required property var activeScene
    required property var activeSceneEnvironment

    property alias view3d: view3D

    implicitWidth: view3D.dispSize.width
    implicitHeight: view3D.dispSize.height
    clip: true

    View3D {
        id: view3D

        readonly property bool isOrthographicCamera: _generalHelper.isOrthographicCamera(view3D.camera)
        property size dispSize: calculateSize(cameraDisplay.viewPortSize, cameraDisplay.preferredSize)
        readonly property real magnificationFactor: cameraDisplay.viewPortSize.width === 0
                                                    ? 1.0
                                                    : (view3D.dispSize.width / cameraDisplay.viewPortSize.width)

        transformOrigin: Item.Center
        anchors.centerIn: parent

        camera: cameraDisplay.camera
        importScene: cameraDisplay.activeScene
        environment: cameraDisplay.activeSceneEnvironment ?? defaultSceneEnvironment

        function calculateSize(viewPortSize: size, preferredSize : size) {
            if (_generalHelper.fuzzyCompare(viewPortSize.width, 0)
                    || _generalHelper.fuzzyCompare(viewPortSize.height, 0)) {
                return Qt.size(0, 0)
            }

            let aspectRatio = viewPortSize.height / viewPortSize.width
            var calculatedHeight = preferredSize.width * aspectRatio
            if (calculatedHeight <= preferredSize.height)
                return Qt.size(preferredSize.width, calculatedHeight)

            var calculatedWidth = preferredSize.height / aspectRatio;
            return Qt.size(calculatedWidth, preferredSize.height);
        }
    }

    SceneEnvironment {
        id: defaultSceneEnvironment

        antialiasingMode: SceneEnvironment.MSAA
        antialiasingQuality: SceneEnvironment.High
    }

    states: [
        State {
            name: "orthoCamera"
            when: view3D.isOrthographicCamera
            PropertyChanges {
                target: view3D

                width: cameraDisplay.viewPortSize.width
                height: cameraDisplay.viewPortSize.height
                scale: view3D.magnificationFactor
            }
        },
        State {
            name: "nonOrthoCamera"
            when: !view3D.isOrthographicCamera
            PropertyChanges {
                target: view3D

                width: view3D.dispSize.width
                height: view3D.dispSize.height
                scale: 1.0
            }
        }
    ]
}
