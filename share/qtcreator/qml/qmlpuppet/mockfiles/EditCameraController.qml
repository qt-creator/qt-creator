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
import QtQuick3D 1.0

Item {
    id: cameraCtrl

    property Camera camera: null
    property View3D view3d: null

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
    property real _defaultCameraLookAtDistance: 0
    property Camera _prevCamera: null

    function focusObject(targetObject, rotation, updateZoom)
    {
        camera.rotation = rotation;
        var newLookAtAndZoom = _generalHelper.focusObjectToCamera(
                    camera, _defaultCameraLookAtDistance, targetObject, view3d, _zoomFactor, updateZoom);
        _lookAtPoint = newLookAtAndZoom.toVector3d();
        _zoomFactor = newLookAtAndZoom.w;
    }

    function zoomRelative(distance)
    {
        _zoomFactor = _generalHelper.zoomCamera(camera, distance, _defaultCameraLookAtDistance,
                                                _lookAtPoint, _zoomFactor, true);
    }

    Component.onCompleted: {
        cameraCtrl._defaultCameraLookAtDistance = cameraCtrl.camera.position.length();
    }

    onCameraChanged: {
        if (_prevCamera) {
            // Reset zoom on previous camera to ensure it's properties are good to copy to new cam
            _generalHelper.zoomCamera(_prevCamera, 0, _defaultCameraLookAtDistance, _lookAtPoint,
                                      1, false);

            camera.position = _prevCamera.position;
            camera.rotation = _prevCamera.rotation;

            // Apply correct zoom to new camera
            _generalHelper.zoomCamera(camera, 0, _defaultCameraLookAtDistance, _lookAtPoint,
                                      _zoomFactor, false);
        }
        _prevCamera = camera;
    }

    MouseArea {
        id: mouseHandler
        acceptedButtons: Qt.LeftButton | Qt.RightButton | Qt.MiddleButton
        hoverEnabled: false
        anchors.fill: parent
        onPositionChanged: {
            if (mouse.modifiers === Qt.AltModifier && cameraCtrl._dragging) {
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
        onPressed: {
            if (mouse.modifiers === Qt.AltModifier) {
                cameraCtrl._dragging = true;
                cameraCtrl._startRotation = cameraCtrl.camera.rotation;
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

        onReleased: cameraCtrl._dragging = false;
        onCanceled: cameraCtrl._dragging = false;
        onWheel: {
            // Emprically determined divisor for nice zoom
            cameraCtrl.zoomRelative(wheel.angleDelta.y / -40);
        }
    }
}
