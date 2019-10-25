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

    property alias scene: editView.scene
    property alias showEditLight: editLightCheckbox.checked
    property alias usePerspective: usePerspectiveCheckbox.checked

    property Node selectedNode: null

    signal objectClicked(var object)
    signal commitObjectPosition(var object)
    signal moveObjectPosition(var object)

    function selectObject(object) {
        selectedNode = object;
    }

    Node {
        id: overlayScene

        Camera {
            id: overlayCamera
            projectionMode: usePerspectiveCheckbox.checked ? Camera.Perspective
                                                           : Camera.Orthographic
            clipFar: editCamera.clipFar
            position: editCamera.position
            rotation: editCamera.rotation
        }

        MoveGizmo {
            id: moveGizmo
            scale: autoScale.getScale(Qt.vector3d(5, 5, 5))
            highlightOnHover: true
            targetNode: viewWindow.selectedNode
            position: viewWindow.selectedNode ? viewWindow.selectedNode.scenePosition
                                              : Qt.vector3d(0, 0, 0)

            rotation: globalControl.checked || !viewWindow.selectedNode
                      ? Qt.vector3d(0, 0, 0)
                      : viewWindow.selectedNode.sceneRotation

            visible: selectedNode
            view3D: overlayView

            onPositionCommit: viewWindow.commitObjectPosition(selectedNode)
            onPositionMove: viewWindow.moveObjectPosition(selectedNode)
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
                viewWindow.objectClicked(pickResult.objectHit);
                selectObject(pickResult.objectHit);
            }
        }

        View3D {
            id: editView
            anchors.fill: parent
            camera: editCamera

            Node {
                id: mainSceneHelpers

                AxisHelper {
                    id: axisGrid
                    enableXZGrid: true
                    enableAxisLines: false
                }

                PointLight {
                    id: pointLight
                    visible: showEditLight
                    position: editCamera.position
                }

                Camera {
                    id: editCamera
                    y: 200
                    z: -300
                    clipFar: 100000
                    projectionMode: usePerspective ? Camera.Perspective : Camera.Orthographic
                }
            }
        }

        View3D {
            id: overlayView
            anchors.fill: parent
            camera: overlayCamera
            scene: overlayScene
        }

        Overlay2D {
            id: gizmoLabel
            targetNode: moveGizmo
            targetView: overlayView
            offsetX: 0
            offsetY: 45
            visible: moveGizmo.isDragging

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
                        selectedNode
                            ? qsTr("x:") + Number(selectedNode.position.x).toLocaleString(l, 'f', 1)
                              + qsTr(" y:") + Number(selectedNode.position.y).toLocaleString(l, 'f', 1)
                              + qsTr(" z:") + Number(selectedNode.position.z).toLocaleString(l, 'f', 1)
                            : "";
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
            text: qsTr("Use global orientation")
            onCheckedChanged: cameraControl.forceActiveFocus()
        }
    }

    Text {
        id: helpText
        text: qsTr("Camera: W,A,S,D,R,F,right mouse drag")
        anchors.bottom: parent.bottom
    }
}
