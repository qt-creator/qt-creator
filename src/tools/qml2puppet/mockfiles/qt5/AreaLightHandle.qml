// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.0
import QtQuick3D 1.15

DirectionalDraggable {
    id: handleRoot

    property string currentLabel
    property point currentMousePos
    property string propName
    property real propValue: 0
    property real newValue: 0
    property real baseScale: 5

    scale: autoScaler.getScale(Qt.vector3d(baseScale, baseScale, baseScale))
    length: 3
    offset: -1.5

    Model {
        id: handle
        source: "#Sphere"
        materials: [ handleRoot.material ]
        scale: Qt.vector3d(0.02, 0.02, 0.02)
    }

    AutoScaleHelper {
        id: autoScaler
        active: handleRoot.active
        view3D: handleRoot.view3D
    }

    property real _startValue
    property real _startScale

    signal valueCommit()
    signal valueChange()

    function updateValue(relativeDistance, screenPos)
    {
        handleRoot.newValue = Math.round(Math.min(999999, Math.max(0, _startValue + (relativeDistance * _startScale))));
        var l = Qt.locale();
        handleRoot.currentLabel = propName + qsTr(": ") + Number(newValue).toLocaleString(l, 'f', 0);
        handleRoot.currentMousePos = screenPos;
    }

    onPressed: (mouseArea, screenPos)=> {
        _startScale = autoScaler.relativeScale * baseScale;
        _startValue = propValue;
        updateValue(0, screenPos);
    }

    onDragged: (mouseArea, sceneRelativeDistance, relativeDistance, screenPos)=> {
        updateValue(relativeDistance, screenPos);
        handleRoot.valueChange();
    }

    onReleased: (mouseArea, sceneRelativeDistance, relativeDistance, screenPos)=> {
        updateValue(relativeDistance, screenPos);
        handleRoot.valueCommit();
    }
}
