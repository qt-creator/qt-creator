// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 6.0
import QtQuick3D 6.0

Item {
    id: cameraCtrl

    property Camera camera: null
    property View3D view3d: null
    property string sceneId
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
    readonly property real _keyPanAmount: 5
    property bool ignoreToolState: false

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
        camera.position = _defaultCameraPosition;
        camera.eulerRotation = _defaultCameraRotation;
        _generalHelper.zoomCamera(view3d, camera, 0, _defaultCameraLookAtDistance, _lookAtPoint,
                                  _zoomFactor, false);
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
        _generalHelper.storeToolState(sceneId, "editCamState", cameraState, delay);
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

        _lookAtPoint = _generalHelper.alignView(camera, nodes, _lookAtPoint);
        storeCameraState(0);
    }

    function zoomRelative(distance)
    {
        if (!camera)
            return;

        _zoomFactor = _generalHelper.zoomCamera(view3d, camera, distance, _defaultCameraLookAtDistance,
                                                _lookAtPoint, _zoomFactor, true);
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

    MouseArea {
        id: mouseHandler
        acceptedButtons: Qt.LeftButton | Qt.RightButton | Qt.MiddleButton
        hoverEnabled: false
        anchors.fill: parent
        onPositionChanged: (mouse)=> {
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
        onPressed: (mouse)=> {
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

        function handleRelease() {
            cameraCtrl._dragging = false;
            cameraCtrl.storeCameraState(0);
        }

        onReleased: handleRelease()
        onCanceled: handleRelease()

        onWheel: (wheel)=> {
            if (cameraCtrl.camera) {
                // Empirically determined divisor for nice zoom
                cameraCtrl.zoomRelative(wheel.angleDelta.y / -40);
                cameraCtrl.storeCameraState(500);
            }
        }
    }

    Keys.onPressed: (event)=> {
        var pressPoint = Qt.vector3d(view3d.width / 2, view3d.height / 2, 0);
        var currentPoint;

        switch (event.key) {
        case Qt.Key_Left:
            currentPoint = pressPoint.plus(Qt.vector3d(_keyPanAmount, 0, 0));
            break;
        case Qt.Key_Right:
            currentPoint = pressPoint.plus(Qt.vector3d(-_keyPanAmount, 0, 0));
            break;
        case Qt.Key_Up:
            currentPoint = pressPoint.plus(Qt.vector3d(0, _keyPanAmount, 0));
            break;
        case Qt.Key_Down:
            currentPoint = pressPoint.plus(Qt.vector3d(0, -_keyPanAmount, 0));
            break;
        default:
            break;
        }

        if (currentPoint) {
            _lookAtPoint = _generalHelper.panCamera(
                        camera, cameraCtrl.camera.sceneTransform,
                        cameraCtrl.camera.position, _lookAtPoint,
                        pressPoint, currentPoint, _zoomFactor);
            event.accepted = true;
        }
    }
}
