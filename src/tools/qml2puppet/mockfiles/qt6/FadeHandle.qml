// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 6.0
import QtQuick3D 6.0

DirectionalDraggable {
    id: handleRoot

    property string currentLabel
    property point currentMousePos
    property real fadeScale
    property real baseScale: 5
    property real dragScale: 1

    scale: autoScaler.getScale(Qt.vector3d(baseScale, baseScale, baseScale))
    length: 3
    offset: -1.5

    Model {
        id: handle
        readonly property bool _edit3dLocked: true // Make this non-pickable
        source: "#Sphere"
        materials: [ handleRoot.material ]
        scale: Qt.vector3d(0.02, 0.02, 0.02)
    }

    AutoScaleHelper {
        id: autoScaler
        active: handleRoot.active
        view3D: handleRoot.view3D
    }

    property real _q // quadratic fade
    property real _l // linear fade
    property real _c // constant fade
    property real _d: 20  // Divisor from fadeScale calc in lightGizmo
    property real _startScale
    property real _startFadeScale
    property string _currentProp

    signal valueCommit(string propName)
    signal valueChange(string propName)

    function updateFade(relativeDistance, screenPos)
    {
        // Solved from fadeScale equation in LightGizmo
        var newValue = 0;
        var _x = Math.max(0, (_startFadeScale - (relativeDistance * _startScale)) / 100);

        // Fades capped to range 0-10 because property editor caps them to that range
        if (_currentProp === "quadraticFade") {
            if (_x === 0)
                newValue = 10;
            else
                newValue = Math.max(0, Math.min(10, -(_c - _d + (_l * _x)) / (_x * _x)));
            if (newValue < 0.01)
                newValue = 0; // To avoid having tiny positive value when UI shows 0.00
            targetNode.quadraticFade = newValue;
        } else if (_currentProp === "linearFade") {
            if (_x === 0)
                newValue = 10;
            else
                newValue = Math.max(0, Math.min(10, -(_c - _d) / _x));
            if (newValue < 0.01)
                newValue = 0; // To avoid having tiny positive value when UI shows 0.00
            targetNode.linearFade = newValue;
        } else {
            // Since pure constant fade equates to infinitely long cone, fadeScale calc assumes
            // linear fade of one in this case.
            newValue = Math.max(0, Math.min(10, _d - _x));
            targetNode.constantFade = newValue;
        }

        var l = Qt.locale();
        handleRoot.currentLabel = _currentProp + qsTr(": ") + Number(newValue).toLocaleString(l, 'f', 2);
        handleRoot.currentMousePos = screenPos;
    }

    onPressed: (mouseArea, screenPos)=> {
        _startScale = autoScaler.relativeScale * baseScale * dragScale;
        _startFadeScale = fadeScale;
        _l = targetNode.linearFade;
        _c = targetNode.constantFade;
        _q = targetNode.quadraticFade;
        if (targetNode.quadraticFade === 0) {
            if (targetNode.linearFade === 0) {
                _currentProp = "constantFade";
            } else {
                _currentProp = "linearFade";
            }
        } else {
            _currentProp = "quadraticFade";
        }
        updateFade(0, screenPos);
    }

    onDragged: (mouseArea, sceneRelativeDistance, relativeDistance, screenPos)=> {
        updateFade(relativeDistance, screenPos);
        handleRoot.valueChange(_currentProp);
    }

    onReleased: (mouseArea, sceneRelativeDistance, relativeDistance, screenPos)=> {
        updateFade(relativeDistance, screenPos);
        handleRoot.valueCommit(_currentProp);
    }
}
