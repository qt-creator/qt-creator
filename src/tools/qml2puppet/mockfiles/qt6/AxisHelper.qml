// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 6.0
import QtQuick3D 6.0

View3D {
    id: axisHelperView

    property var editCameraCtrl
    property Node selectedNode

    camera: axisHelperCamera

    Node {
        OrthographicCamera {
            id: axisHelperCamera
            rotation: editCameraCtrl.camera ? editCameraCtrl.camera.rotation : Qt.quaternion(1, 0, 0, 0)
            position: editCameraCtrl.camera ? editCameraCtrl.camera.position.minus(editCameraCtrl._lookAtPoint)
                                              .normalized().times(600) : Qt.vector3d(0, 0, 0)
        }

        AutoScaleHelper {
            id: autoScale
            view3D: axisHelperView
            position: axisHelperGizmo.scenePosition
        }

        Node {
            id: axisHelperGizmo
            scale: autoScale.getScale(Qt.vector3d(4, 4, 4))

            AxisHelperArm {
                id: armX
                eulerRotation: Qt.vector3d(0, 0, -90)
                color: Qt.rgba(1, 0, 0, 1)
                hoverColor: Qt.lighter(Qt.rgba(1, 0, 0, 1))
                view3D: axisHelperView
                camRotPos: Qt.vector3d(0, 90, 0)
                camRotNeg: Qt.vector3d(0, -90, 0)
            }

            AxisHelperArm {
                id: armY
                eulerRotation: Qt.vector3d(0, 0, 0)
                color: Qt.rgba(0, 0.6, 0, 1)
                hoverColor: Qt.lighter(Qt.rgba(0, 0.6, 0, 1))
                view3D: axisHelperView
                camRotPos: Qt.vector3d(-90, 0, 0)
                camRotNeg: Qt.vector3d(90, 0, 0)
            }

            AxisHelperArm {
                id: armZ
                eulerRotation: Qt.vector3d(90, 0, 0)
                color: Qt.rgba(0, 0, 1, 1)
                hoverColor: Qt.lighter(Qt.rgba(0, 0, 1, 1))
                view3D: axisHelperView
                camRotPos: Qt.vector3d(0, 0, 0)
                camRotNeg: Qt.vector3d(0, 180, 0)
            }
        }
    }

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton

        property var pickObj: null

        function cancelHover()
        {
            if (pickObj) {
                pickObj.hovering = false;
                pickObj = null;
            }
        }

        function pick(mouse)
        {
            var result = axisHelperView.pick(mouse.x, mouse.y);
            if (result.objectHit) {
                if (result.objectHit !== pickObj) {
                    cancelHover();
                    pickObj = result.objectHit;
                    pickObj.hovering = true;
                }
            } else {
                cancelHover();
            }
        }

        onPositionChanged: (mouse)=> {
            pick(mouse);
        }

        onPressed: (mouse)=> {
            pick(mouse);
            if (pickObj) {
                axisHelperView.editCameraCtrl.focusObject(axisHelperView.selectedNode,
                                                          pickObj.cameraRotation, false, false);
            } else {
                mouse.accepted = false;
            }
        }

        onExited: cancelHover()
        onCanceled: cancelHover()
    }
}
