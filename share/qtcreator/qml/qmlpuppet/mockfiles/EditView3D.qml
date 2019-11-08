/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

import QtQuick 2.12
import QtQuick.Window 2.0
import QtQuick3D 1.0
import QtQuick3D.Helpers 1.0
import QtQuick.Controls 2.0
import QtGraphicalEffects 1.0

Window {
    id: viewWindow
    width: 1024
    height: 768
    visible: true
    title: "3D"
    flags: Qt.WindowStaysOnTopHint | Qt.Window | Qt.WindowTitleHint | Qt.WindowCloseButtonHint

    property alias scene: editView.importScene
    property alias showEditLight: editLightCheckbox.checked
    property alias usePerspective: usePerspectiveCheckbox.checked

    property Node selectedNode: null

    property var lightGizmos: []
    property var cameraGizmos: []

    signal objectClicked(var object)
    signal commitObjectProperty(var object, var propName)
    signal changeObjectProperty(var object, var propName)

    function selectObject(object) {
        selectedNode = object;
    }

    function emitObjectClicked(object) {
        selectObject(object);
        objectClicked(object);
    }

    function addLightGizmo(obj)
    {
        var component = Qt.createComponent("LightGizmo.qml");
        if (component.status === Component.Ready) {
            var gizmo = component.createObject(overlayScene,
                                               {"view3D": overlayView, "targetNode": obj});
            lightGizmos[lightGizmos.length] = gizmo;
            gizmo.selected.connect(emitObjectClicked);
        }
    }

    function addCameraGizmo(obj)
    {
        var component = Qt.createComponent("CameraGizmo.qml");
        if (component.status === Component.Ready) {
            var gizmo = component.createObject(overlayScene,
                                               {"view3D": overlayView, "targetNode": obj});
            cameraGizmos[cameraGizmos.length] = gizmo;
            gizmo.selected.connect(emitObjectClicked);
        }
    }

    // Work-around the fact that the projection matrix for the camera is not calculated until
    // the first frame is rendered, so any initial calls to mapFrom3DScene() will fail.
    Component.onCompleted: designStudioNativeCameraControlHelper.requestOverlayUpdate();

    onWidthChanged: designStudioNativeCameraControlHelper.requestOverlayUpdate();
    onHeightChanged: designStudioNativeCameraControlHelper.requestOverlayUpdate();

    Node {
        id: overlayScene

        PerspectiveCamera {
            id: overlayPerspectiveCamera
            clipFar: editPerspectiveCamera.clipFar
            position: editPerspectiveCamera.position
            rotation: editPerspectiveCamera.rotation
        }

        OrthographicCamera {
            id: overlayOrthoCamera
            position: editOrthoCamera.position
            rotation: editOrthoCamera.rotation
        }

        MoveGizmo {
            id: moveGizmo
            scale: autoScale.getScale(Qt.vector3d(5, 5, 5))
            highlightOnHover: true
            targetNode: viewWindow.selectedNode
            position: viewWindow.selectedNode ? viewWindow.selectedNode.scenePosition
                                              : Qt.vector3d(0, 0, 0)
            globalOrientation: globalControl.checked
            visible: selectedNode && moveToolControl.checked
            view3D: overlayView

            onPositionCommit: viewWindow.commitObjectProperty(selectedNode, "position")
            onPositionMove: viewWindow.changeObjectProperty(selectedNode, "position")
        }

        ScaleGizmo {
            id: scaleGizmo
            scale: autoScale.getScale(Qt.vector3d(5, 5, 5))
            highlightOnHover: true
            targetNode: viewWindow.selectedNode
            position: viewWindow.selectedNode ? viewWindow.selectedNode.scenePosition
                                              : Qt.vector3d(0, 0, 0)
            globalOrientation: globalControl.checked
            visible: selectedNode && scaleToolControl.checked
            view3D: overlayView

            onScaleCommit: viewWindow.commitObjectProperty(selectedNode, "scale")
            onScaleChange: viewWindow.changeObjectProperty(selectedNode, "scale")
        }

        AutoScaleHelper {
            id: autoScale
            view3D: overlayView
            position: moveGizmo.scenePosition
        }
    }

    Rectangle {
        id: sceneBg
        color: "#FFFFFF"
        anchors.fill: parent
        focus: true

        TapHandler { // check tapping/clicking an object in the scene
            onTapped: {
                var pickResult = editView.pick(eventPoint.scenePosition.x,
                                               eventPoint.scenePosition.y);
                emitObjectClicked(pickResult.objectHit);
            }
        }

        View3D {
            id: editView
            anchors.fill: parent
            camera: usePerspective ? editPerspectiveCamera : editOrthoCamera

            Node {
                id: mainSceneHelpers

                AxisHelper {
                    id: axisGrid
                    enableXZGrid: true
                    enableAxisLines: false
                }

                PointLight {
                    id: editLight
                    visible: showEditLight
                    position: usePerspective ? editPerspectiveCamera.position
                                             : editOrthoCamera.position
                    quadraticFade: 0
                    linearFade: 0
                }

                PerspectiveCamera {
                    id: editPerspectiveCamera
                    y: 200
                    z: -300
                    clipFar: 100000
                }

                OrthographicCamera {
                    id: editOrthoCamera
                    y: 200
                    z: -300
                }
            }
        }

        View3D {
            id: overlayView
            anchors.fill: parent
            camera: usePerspective ? overlayPerspectiveCamera : overlayOrthoCamera
            importScene: overlayScene
        }

        Overlay2D {
            id: gizmoLabel
            targetNode: moveGizmo.visible ? moveGizmo : scaleGizmo
            targetView: overlayView
            offsetX: 0
            offsetY: 45
            visible: targetNode.dragging

            Rectangle {
                color: "white"
                x: -width / 2
                y: -height
                width: gizmoLabelText.width + 4
                height: gizmoLabelText.height + 4
                border.width: 1
                Text {
                    id: gizmoLabelText
                    text: {
                        var l = Qt.locale();
                        var targetProperty;
                        if (viewWindow.selectedNode) {
                            if (gizmoLabel.targetNode === moveGizmo)
                                targetProperty = viewWindow.selectedNode.position;
                            else
                                targetProperty = viewWindow.selectedNode.scale;
                            return qsTr("x:") + Number(targetProperty.x).toLocaleString(l, 'f', 1)
                                + qsTr(" y:") + Number(targetProperty.y).toLocaleString(l, 'f', 1)
                                + qsTr(" z:") + Number(targetProperty.z).toLocaleString(l, 'f', 1);
                        } else {
                            return "";
                        }
                    }
                    anchors.centerIn: parent
                }
            }
        }

        WasdController {
            id: cameraControl
            controlledObject: editView.camera
            acceptedButtons: Qt.RightButton

            onInputsNeedProcessingChanged: designStudioNativeCameraControlHelper.enabled
                                           = cameraControl.inputsNeedProcessing

            // Use separate native timer as QML timers don't work inside Qt Design Studio
            Connections {
                target: designStudioNativeCameraControlHelper
                onUpdateInputs: cameraControl.processInputs()
            }
        }
    }

    Column {
        y: 8
        CheckBox {
            id: editLightCheckbox
            checked: false
            text: qsTr("Use Edit View Light")
            onCheckedChanged: cameraControl.forceActiveFocus()
        }

        CheckBox {
            id: usePerspectiveCheckbox
            checked: true
            text: qsTr("Use Perspective Projection")
            onCheckedChanged: cameraControl.forceActiveFocus()
        }

        CheckBox {
            id: globalControl
            checked: true
            text: qsTr("Use Global Orientation")
            onCheckedChanged: cameraControl.forceActiveFocus()
        }
        Column {
            x: 8
            RadioButton {
                id: moveToolControl
                checked: true
                text: qsTr("Move Tool")
            }
            RadioButton {
                id: scaleToolControl
                checked: false
                text: qsTr("Scale Tool")
            }
        }
    }

    Text {
        id: helpText
        text: qsTr("Camera: W,A,S,D,R,F,right mouse drag")
        anchors.bottom: parent.bottom
    }
}
