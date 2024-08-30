// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
import QtQuick
import QtQuick3D

Rectangle {
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
    property View3D view3d

    width: priv.loaderSize.width + 2
    height: priv.loaderSize.height + 2

    anchors.bottom: parent.bottom
    anchors.margins: 10

    visible: priv.cameraViewIsOn

    onTargetNodeChanged: priv.updateCamera()
    onAlwaysOnChanged: priv.updateCamera()
    onPreferredCameraChanged: priv.updateCamera()
    onActiveSceneChanged: forceReload()
    Component.onCompleted: priv.updateCamera()

    border.width: 1
    border.color: "lightslategray"
    color: "black"

    function forceReload() {
        priv.forceDeactive = true
        priv.oldCamera = null
        priv.updateCamera()
        priv.forceDeactive = false
    }

    QtObject {
        id: priv

        property Camera camera
        property Camera oldCamera
        property bool cameraIsSceneObject
        property bool forceDeactive: false
        readonly property bool cameraViewIsOn: !forceDeactive && (cameraView.alwaysOn || cameraView.showCameraView) && priv.camera
        readonly property bool cameraHasValidScene: priv.cameraViewIsOn && priv.cameraIsSceneObject
        property Loader activeLoader
        readonly property size loaderSize: activeLoader && activeLoader.active
                                           ? Qt.size(activeLoader.width, activeLoader.height)
                                           : Qt.size(-2, -2)

        function updateCamera() {
            let activeCam = activeCamera()
            priv.camera = activeCam
            priv.cameraIsSceneObject = _generalHelper.isSceneObject(activeCam)
        }

        function activeCamera() {
            if (cameraView.alwaysOn) {
                if (_generalHelper.isCamera(cameraView.targetNode))
                    return cameraView.targetNode
                else if (priv.oldCamera)
                    return priv.oldCamera
                else
                    return cameraView.preferredCamera
            } else if (_generalHelper.isCamera(cameraView.targetNode)) {
                return cameraView.targetNode
            }
            return null
        }

        onCameraChanged: {
            if (priv.camera)
                priv.oldCamera = priv.camera
        }
    }

    Loader {
        id: cameraLoader

        active: priv.cameraViewIsOn && priv.cameraHasValidScene
        onLoaded: priv.activeLoader = this
        sourceComponent: CameraDisplay {
            id: cameraDisplay
            camera: priv.camera
            preferredSize: cameraView.preferredSize
            viewPortSize: cameraView.viewPortSize
            activeScene: cameraView.activeScene
            activeSceneEnvironment: cameraView.activeSceneEnvironment

            Binding {
                target: cameraView
                property: "view3d"
                value: cameraDisplay.view3d
            }
        }
    }

    Loader {
        id: errorLoader

        active: priv.cameraViewIsOn && !priv.cameraHasValidScene
        onLoaded: priv.activeLoader = this
        sourceComponent: Text {
            font.pixelSize: 14
            color: "yellow"
            text: qsTr("Camera does not have a valid view")
            padding: 10
        }
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
