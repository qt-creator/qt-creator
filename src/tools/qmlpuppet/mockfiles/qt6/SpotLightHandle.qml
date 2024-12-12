// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 6.0
import QtQuick3D 6.0

DirectionalDraggable {
    id: handleRoot

    property string currentLabel
    property point currentMousePos
    property string propName
    property real propValue: 0
    property real newValue: 0

    scale: autoScaler.getScale(Qt.vector3d(5, 5, 5))
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

    property real _startAngle

    signal valueCommit()
    signal valueChange()

    function updateAngle(relativeDistance, screenPos)
    {
        handleRoot.newValue = Math.round(Math.min(180, Math.max(0, _startAngle + relativeDistance)));
        var l = Qt.locale();
        handleRoot.currentLabel = propName + qsTr(": ") + Number(newValue).toLocaleString(l, 'f', 0);
        handleRoot.currentMousePos = screenPos;
    }

    onPressed: (mouseArea, screenPos)=> {
        _startAngle = propValue;
        updateAngle(0, screenPos);
    }

    onDragged: (mouseArea, sceneRelativeDistance, relativeDistance, screenPos)=> {
        updateAngle(relativeDistance, screenPos);
        handleRoot.valueChange();
    }

    onReleased: (mouseArea, sceneRelativeDistance, relativeDistance, screenPos)=> {
        updateAngle(relativeDistance, screenPos);
        handleRoot.valueCommit();
    }
}
