// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 6.0
import QtQuick3D 6.0
import MouseArea3D 1.0

Node {
    id: gizmo

    readonly property bool dragging: fovArrow.dragging
                                      || clipFarHandle.dragging
                                      || clipNearHandle.dragging
    property View3D view3D
    property Node targetNode: null
    property MouseArea3D dragHelper: null
    property point currentMousePos
    property string currentLabel

    visible: (gizmo.targetNode instanceof OrthographicCamera || gizmo.targetNode instanceof PerspectiveCamera)
    position: gizmo.targetNode !== null ? targetNode.scenePosition : Qt.vector3d(0, 0, 0)
    rotation: gizmo.targetNode !== null ? targetNode.sceneRotation : Qt.quaternion(1, 0, 0, 0)

    AutoScaleHelper {
        id: autoScaler
        view3D: gizmo.view3D
    }

    CameraFrustumHandle {
        id: clipFarHandle
        view3D: gizmo.view3D
        targetNode: gizmo.targetNode
        dragHelper: gizmo.dragHelper
        color: (hovering || dragging) ? Qt.rgba(1, 1, 1, 1) : "#FF0000"
        propName: "clipFar"

        onCurrentMousePosChanged: {
            gizmo.currentMousePos = clipFarHandle.currentMousePos;
            gizmo.currentLabel = clipFarHandle.currentLabel;
        }
    }

    AdjustableArrow {
        id: fovArrow
        readonly property bool isPerspectiveCamera: gizmo.targetNode instanceof PerspectiveCamera
        view3D: gizmo.view3D
        targetNode: gizmo.targetNode
        dragHelper: gizmo.dragHelper
        color: (hovering || dragging) ? Qt.rgba(1, 1, 1, 1) : "#FF0000"
        scale: autoScaler.getScale(Qt.vector3d(5, 5, 5))
        active: fovArrow.isPerspectiveCamera
        visible: fovArrow.isPerspectiveCamera
        z: fovArrow.isPerspectiveCamera ? -(gizmo.targetNode.clipNear + 0.5 * (gizmo.targetNode.clipFar - gizmo.targetNode.clipNear)) : 0
        eulerRotation: fovArrow.isPerspectiveCamera && (gizmo.targetNode.fieldOfViewOrientation === PerspectiveCamera.Horizontal)
                        ? Qt.vector3d(0, 0, -90)
                        : Qt.vector3d(0, 0, 0)
        length: isPerspectiveCamera ? 0.5 * targetNode.fieldOfView : 10

        property real _startValue

        function updateValue(relativeDistance, screenPos)
        {
            const value = Math.max(1, Math.min(_startValue + relativeDistance, 180));
            gizmo.targetNode.fieldOfView = value;

            var l = Qt.locale();
            gizmo.currentLabel = "fieldOfView" + qsTr(": ") + Number(value).toLocaleString(l, 'f', 2);
            gizmo.currentMousePos = screenPos;
        }

        onPressed: (mouseArea, screenPos)=> {
            _startValue = gizmo.targetNode.fieldOfView;
            updateValue(0, screenPos);
        }

        onDragged: (mouseArea, sceneRelativeDistance, relativeDistance, screenPos)=> {
            updateValue(relativeDistance, screenPos);
            gizmo.view3D.changeObjectProperty([gizmo.targetNode], ["fieldOfView"]);
        }

        onReleased: (mouseArea, sceneRelativeDistance, relativeDistance, screenPos)=> {
            updateValue(relativeDistance, screenPos);
            gizmo.view3D.commitObjectProperty([gizmo.targetNode], ["fieldOfView"]);
        }
    }

    CameraFrustumHandle {
        id: clipNearHandle
        view3D: gizmo.view3D
        targetNode: gizmo.targetNode
        dragHelper: gizmo.dragHelper
        color: (hovering || dragging) ? Qt.rgba(1, 1, 1, 1) : "#FF0000"
        propName: "clipNear"

        onCurrentMousePosChanged: {
            gizmo.currentMousePos = clipNearHandle.currentMousePos;
            gizmo.currentLabel = clipNearHandle.currentLabel;
        }
    }
}
