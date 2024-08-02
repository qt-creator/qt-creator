// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
import QtQuick
import QtQuick3D

Item {
    id: cameraView

    required property bool showCameraView
    required property bool alwaysOn
    property bool snapLeft: true
    property Node targetNode
    property size preferredSize
    property size viewPortSize
    property var activeScene
    property var activeSceneEnvironment
    property var preferredCamera

    width: loader.width
    height: loader.height
    anchors.bottom: parent.bottom
    anchors.margins: 10
    visible: loader.active

    onTargetNodeChanged: loader.updateCamera()
    onAlwaysOnChanged: loader.updateCamera()
    onPreferredCameraChanged: loader.updateCamera()
    onActiveSceneChanged: forceReload()
    Component.onCompleted: loader.updateCamera()

    function forceReload() {
        loader.forceDeactive = true
        loader.oldCamera = null
        loader.updateCamera()
        loader.forceDeactive = false
    }

    Loader {
        id: loader

        property Camera camera
        property Camera oldCamera
        property bool forceDeactive: false

        active: !forceDeactive && (cameraView.alwaysOn || cameraView.showCameraView) && loader.camera

        function updateCamera() {
            loader.camera = activeCamera()
        }

        function activeCamera() {
            if (cameraView.alwaysOn) {
                if (_generalHelper.isCamera(cameraView.targetNode))
                    return cameraView.targetNode
                else if (loader.oldCamera)
                    return loader.oldCamera
                else
                    return cameraView.preferredCamera
            } else if (_generalHelper.isCamera(cameraView.targetNode)) {
                return cameraView.targetNode
            }
            return null
        }

        onCameraChanged: {
            if (loader.camera)
                loader.oldCamera = loader.camera
        }

        sourceComponent: Rectangle {
            id: cameraRect

            implicitWidth: view3D.dispSize.width + 2
            implicitHeight: view3D.dispSize.height + 2
            border.width: 1
            border.color: "lightslategray"
            color: "black"

            View3D {
                id: view3D

                readonly property bool isOrthographicCamera: _generalHelper.isOrthographicCamera(view3D.camera)
                property size dispSize: calculateSize(cameraView.viewPortSize, cameraView.preferredSize)
                readonly property real magnificationFactor: cameraView.viewPortSize.width === 0
                                                            ? 1.0
                                                            : (view3D.dispSize.width / cameraView.viewPortSize.width)

                transformOrigin: Item.Center
                anchors.centerIn: parent

                camera: loader.camera
                importScene: cameraView.activeScene
                environment: cameraView.activeSceneEnvironment ?? defaultSceneEnvironment

                function calculateSize(viewPortSize: size, preferredSize : size){
                    if (_generalHelper.fuzzyCompare(viewPortSize.width, 0)
                            || _generalHelper.fuzzyCompare(viewPortSize.height, 0))
                        return Qt.size(0, 0)

                    let aspectRatio = viewPortSize.height / viewPortSize.width
                    var calculatedHeight = preferredSize.width * aspectRatio
                    if (calculatedHeight <= preferredSize.height)
                        return Qt.size(preferredSize.width, calculatedHeight)

                    var calculatedWidth = preferredSize.height / aspectRatio;
                    return Qt.size(calculatedWidth, preferredSize.height);
                }

                states: [
                    State {
                        name: "orthoCamera"
                        when: view3D.isOrthographicCamera
                        PropertyChanges {
                            target: view3D

                            width: cameraView.viewPortSize.width
                            height: cameraView.viewPortSize.height
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
        }
    }

    SceneEnvironment {
        id: defaultSceneEnvironment

        antialiasingMode: SceneEnvironment.MSAA
        antialiasingQuality: SceneEnvironment.High
    }

    states: [
        State {
            name: "default"
            when: cameraView.snapLeft
            AnchorChanges {
                target: cameraView
                anchors.left: parent.left
                anchors.right: undefined
            }
        },
        State {
            name: "snapRight"
            when: !cameraView.snapLeft
            AnchorChanges {
                target: cameraView
                anchors.left: undefined
                anchors.right: parent.right
            }
        }
    ]
}
