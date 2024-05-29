// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 6.0
import QtQuick3D 6.0

Item {
    id: cameraCtrl

    property var viewRoot: null
    property int splitId: -1
    property Camera camera: view3d ? view3d.camera : null
    property View3D view3d: viewRoot.editViews[splitId]
    property string sceneId: viewRoot.sceneId
    property vector3d _lookAtPoint
    property vector3d _pressPoint
    property vector3d _prevPoint
    property vector3d _startRotation
    property vector3d _startPosition
    property vector3d _startLookAtPoint
    property matrix4x4 _startTransform
    property bool _dragging
    property int _button
    property real _zoomFactor: 1
    property Camera _prevCamera: null
    readonly property vector3d _defaultCameraPosition: Qt.vector3d(0, 600, 600)
    readonly property vector3d _defaultCameraRotation: Qt.vector3d(-45, 0, 0)
    readonly property real _defaultCameraLookAtDistance: _defaultCameraPosition.length()
    readonly property real _keyPanAmount: 1.0
    property bool ignoreToolState: false
    property bool flyMode: viewRoot.flyMode
    property bool showCrosshairs: false

    z: 10
    anchors.fill: parent

    function restoreCameraState(cameraState)
    {
        if (!camera || ignoreToolState)
            return;

        _lookAtPoint = cameraState[0];
        _zoomFactor = cameraState[1];
        camera.position = cameraState[2];
        camera.rotation = cameraState[3];
        _generalHelper.zoomCamera(view3d, camera, 0, _defaultCameraLookAtDistance, _lookAtPoint,
                                  _zoomFactor, false);
    }

    function restoreDefaultState()
    {
        if (!camera)
            return;

        _lookAtPoint = Qt.vector3d(0, 0, 0);
        _zoomFactor = 1;

        if (splitId === 1) {
            jumpToRotation(originGizmo.quaternionForAxis(OriginGizmo.PositiveZ));
        } else if (splitId === 2) {
            jumpToRotation(originGizmo.quaternionForAxis(OriginGizmo.NegativeY));
        } else if (splitId === 3) {
            jumpToRotation(originGizmo.quaternionForAxis(OriginGizmo.NegativeX));
        } else {
            camera.position = _defaultCameraPosition;
            camera.eulerRotation = _defaultCameraRotation;
            _generalHelper.zoomCamera(view3d, camera, 0, _defaultCameraLookAtDistance, _lookAtPoint,
                                      _zoomFactor, false);
        }
    }

    function storeCameraState(delay)
    {
        if (!camera || ignoreToolState)
            return;

        var cameraState = [];
        cameraState[0] = _lookAtPoint;
        cameraState[1] = _zoomFactor;
        cameraState[2] = camera.position;
        cameraState[3] = camera.rotation;
        _generalHelper.storeToolState(sceneId, "editCamState" + splitId, cameraState, delay);
    }

    function focusObject(targetNodes, rotation, updateZoom, closeUp)
    {
        if (!camera)
            return;

        // targetNodes could be a list of nodes or a single node
        var nodes = [];
        if (targetNodes instanceof Node)
            nodes.push(targetNodes);
        else
            nodes = targetNodes

        camera.eulerRotation = rotation;
        var newLookAtAndZoom = _generalHelper.focusNodesToCamera(
                    camera, _defaultCameraLookAtDistance, nodes, view3d, _zoomFactor,
                    updateZoom, closeUp);
        _lookAtPoint = newLookAtAndZoom.toVector3d();
        _zoomFactor = newLookAtAndZoom.w;
        storeCameraState(0);
    }

    function approachObject()
    {
        if (!camera)
            return;

        var pickResult = _generalHelper.pickViewAt(view3d, width / 2, height / 2);
        var resolvedResult = _generalHelper.resolvePick(pickResult.objectHit);

        if (resolvedResult) {
            var newLookAtAndZoom = _generalHelper.approachNode(camera, _defaultCameraLookAtDistance,
                                                               resolvedResult, view3d);
            _lookAtPoint = newLookAtAndZoom.toVector3d();
            _zoomFactor = newLookAtAndZoom.w;
            storeCameraState(0);
        }
    }

    function jumpToRotation(rotation)
    {
        let distance = camera.scenePosition.minus(_lookAtPoint).length()
        let direction = _generalHelper.dirForRotation(rotation)

        camera.rotation = rotation
        camera.position = _lookAtPoint.plus(direction.times(distance))
    }

    function alignCameras(targetNodes)
    {
        if (!camera)
            return;

        // targetNodes could be a list of nodes or a single node
        var nodes = [];
        if (targetNodes instanceof Node)
            nodes.push(targetNodes);
        else
            nodes = targetNodes

        _generalHelper.alignCameras(camera, nodes);
    }

    function alignView(targetNodes)
    {
        if (!camera)
            return;

        // targetNodes could be a list of nodes or a single node
        var nodes = [];
        if (targetNodes instanceof Node)
            nodes.push(targetNodes);
        else
            nodes = targetNodes

        var newLookAtAndZoom = _generalHelper.alignView(camera, nodes, _lookAtPoint,
                                                        _defaultCameraLookAtDistance);
        _lookAtPoint = newLookAtAndZoom.toVector3d();
        _zoomFactor = newLookAtAndZoom.w;
        storeCameraState(0);
    }

    function zoomRelative(distance)
    {
        if (!camera)
            return;

        _zoomFactor = _generalHelper.zoomCamera(view3d, camera, distance, _defaultCameraLookAtDistance,
                                                _lookAtPoint, _zoomFactor, true);
    }

    function rotateCamera(angles)
    {
        if (flyMode)
            showCrosshairs = true;
        cameraCtrl._lookAtPoint = _generalHelper.rotateCamera(camera, angles, _lookAtPoint);
    }

    function moveCamera(moveVec)
    {
        if (flyMode)
            showCrosshairs = true;
        cameraCtrl._lookAtPoint = _generalHelper.moveCamera(camera, _lookAtPoint, moveVec);
    }

    function getMoveVectorForKey(key) {
        if (flyMode) {
            switch (key) {
            case Qt.Key_A:
            case Qt.Key_Left:
                return Qt.vector3d(_keyPanAmount, 0, 0);
            case Qt.Key_D:
            case Qt.Key_Right:
                return Qt.vector3d(-_keyPanAmount, 0, 0);
            case Qt.Key_E:
            case Qt.Key_PageUp:
                return Qt.vector3d(0, _keyPanAmount, 0);
            case Qt.Key_Q:
            case Qt.Key_PageDown:
                return Qt.vector3d(0, -_keyPanAmount, 0);
            case Qt.Key_W:
            case Qt.Key_Up:
                return Qt.vector3d(0, 0, _keyPanAmount);
            case Qt.Key_S:
            case Qt.Key_Down:
                return Qt.vector3d(0, 0, -_keyPanAmount);
            default:
                break;
            }
        } else {
            switch (key) {
            case Qt.Key_Left:
                return Qt.vector3d(_keyPanAmount, 0, 0);
            case Qt.Key_Right:
                return Qt.vector3d(-_keyPanAmount, 0, 0);
            case Qt.Key_Up:
                return Qt.vector3d(0, _keyPanAmount, 0);
            case Qt.Key_Down:
                return Qt.vector3d(0, -_keyPanAmount, 0);
            default:
                break;
            }
        }
        return Qt.vector3d(0, 0, 0);
    }

    onCameraChanged: {
        if (camera && _prevCamera) {
            // Reset zoom on previous camera to ensure it's properties are good to copy to new cam
            _generalHelper.zoomCamera(view3d, _prevCamera, 0, _defaultCameraLookAtDistance, _lookAtPoint,
                                      1, false);

            camera.position = _prevCamera.position;
            camera.rotation = _prevCamera.rotation;

            // Apply correct zoom to new camera
            _generalHelper.zoomCamera(view3d, camera, 0, _defaultCameraLookAtDistance, _lookAtPoint,
                                      _zoomFactor, false);
        }
        _prevCamera = camera;
    }

    onFlyModeChanged: {
        if (cameraCtrl._dragging) {
            cameraCtrl._dragging = false;
            cameraCtrl.storeCameraState(0);
        }
        showCrosshairs = false;
        _generalHelper.stopAllCameraMoves();
        _generalHelper.setCameraSpeedModifier(1.0);
    }

    on_LookAtPointChanged: {
        viewRoot.overlayViews[splitId].lookAtGizmo.position = _lookAtPoint;
    }

    Connections {
        target: _generalHelper
        enabled: viewRoot.activeSplit === cameraCtrl.splitId

        function onRequestCameraMove(camera, moveVec) {
            if (camera === cameraCtrl.camera) {
                cameraCtrl.moveCamera(moveVec);
                _generalHelper.requestRender();
            }
        }
    }

    Image {
        anchors.centerIn: parent
        source: "qrc:///qtquickplugin/mockfiles/images/crosshair.png"
        visible: cameraCtrl.showCrosshairs && viewRoot.activeSplit === cameraCtrl.splitId
        opacity: 0.7
    }

    MouseArea {
        id: mouseHandler
        acceptedButtons: Qt.LeftButton | Qt.RightButton | Qt.MiddleButton
        hoverEnabled: false
        anchors.fill: parent
        onPositionChanged: (mouse) => {
            if (cameraCtrl.camera && mouse.modifiers === Qt.AltModifier && cameraCtrl._dragging) {
                var currentPoint = Qt.vector3d(mouse.x, mouse.y, 0);
                if (cameraCtrl._button == Qt.LeftButton) {
                    _generalHelper.orbitCamera(cameraCtrl.camera, cameraCtrl._startRotation,
                                               cameraCtrl._lookAtPoint, cameraCtrl._pressPoint,
                                               currentPoint);
                } else if (cameraCtrl._button == Qt.MiddleButton) {
                    cameraCtrl._lookAtPoint = _generalHelper.panCamera(
                                cameraCtrl.camera, cameraCtrl._startTransform,
                                cameraCtrl._startPosition, cameraCtrl._startLookAtPoint,
                                cameraCtrl._pressPoint, currentPoint, _zoomFactor);
                } else if (cameraCtrl._button == Qt.RightButton) {
                    cameraCtrl.zoomRelative(currentPoint.y - cameraCtrl._prevPoint.y)
                    cameraCtrl._prevPoint = currentPoint;
                }
            }
        }
        onPressed: (mouse) => {
            if (cameraCtrl.flyMode)
                return;
            viewRoot.activeSplit = cameraCtrl.splitId
            if (cameraCtrl.camera && mouse.modifiers === Qt.AltModifier) {
                cameraCtrl._dragging = true;
                cameraCtrl._startRotation = cameraCtrl.camera.eulerRotation;
                cameraCtrl._startPosition = cameraCtrl.camera.position;
                cameraCtrl._startLookAtPoint = _lookAtPoint;
                cameraCtrl._pressPoint = Qt.vector3d(mouse.x, mouse.y, 0);
                cameraCtrl._prevPoint = cameraCtrl._pressPoint;
                cameraCtrl._button = mouse.button;
                cameraCtrl._startTransform = cameraCtrl.camera.sceneTransform;
            } else {
                mouse.accepted = false;
            }
        }

        function handleRelease()
        {
            cameraCtrl._dragging = false;
            cameraCtrl.storeCameraState(0);
        }

        onReleased: handleRelease()
        onCanceled: handleRelease()

        onWheel: (wheel) => {
            if (cameraCtrl.flyMode && cameraCtrl.splitId !== viewRoot.activeSplit)
                return;
            viewRoot.activeSplit = cameraCtrl.splitId;
            if (cameraCtrl.camera) {
                // Empirically determined divisor for nice zoom
                cameraCtrl.zoomRelative(wheel.angleDelta.y / -40);
                cameraCtrl.storeCameraState(500);
            }
        }
    }

    function setCameraSpeed(event) {
        if (event.modifiers === Qt.AltModifier)
            _generalHelper.setCameraSpeedModifier(0.5);
        else if (event.modifiers === Qt.ShiftModifier)
            _generalHelper.setCameraSpeedModifier(2.0);
        else
            _generalHelper.setCameraSpeedModifier(1.0);
    }

    Keys.onPressed: (event) => {
        setCameraSpeed(event)
        event.accepted = true;
        if (cameraCtrl.flyMode && event.key === Qt.Key_Space)
            approachObject();
        else
            _generalHelper.startCameraMove(cameraCtrl.camera, cameraCtrl.getMoveVectorForKey(event.key));
    }

    Keys.onReleased: (event) => {
        setCameraSpeed(event)
        event.accepted = true;
        _generalHelper.stopCameraMove(cameraCtrl.getMoveVectorForKey(event.key));
    }

    OriginGizmo {
        id: originGizmo
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 4
        width: 70
        height: 70
        targetNode: cameraCtrl.camera

        onAxisClicked: (axis) => {
            viewRoot.activeSplit = cameraCtrl.splitId
            cameraCtrl.jumpToRotation(quaternionForAxis(axis));
        }
    }
}
