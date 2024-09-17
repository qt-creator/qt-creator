// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 6.0
import QtQuick3D 6.0

DirectionalDraggable {
    id: handleRoot

    readonly property bool isGood: handleRoot.targetNode !== null
                                    && (handleRoot.targetNode instanceof OrthographicCamera || handleRoot.targetNode instanceof PerspectiveCamera)
                                    && !!handleRoot.targetNode[propName]
    property string propName
    property string currentLabel
    property point currentMousePos

    active: handleRoot.isGood
    position: handleRoot.isGood ? Qt.vector3d(0, 0, -handleRoot.targetNode[propName])
                                : Qt.vector3d(0, 0, 0)
    eulerRotation: Qt.vector3d(-90, 0, 0)

    scale: autoScaler.getScale(Qt.vector3d(5, 5, 5))
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

    function updateValue(relativeDistance, screenPos)
    {
        const value = _startValue + relativeDistance;
        handleRoot.targetNode[propName] = value;

        var l = Qt.locale();
        handleRoot.currentLabel = handleRoot.propName + qsTr(": ") + Number(value).toLocaleString(l, 'f', 0);
        handleRoot.currentMousePos = screenPos;
    }

    onPressed: (mouseArea, screenPos)=> {
        _startValue = handleRoot.targetNode[propName];
        updateValue(0, screenPos);
    }

    onDragged: (mouseArea, sceneRelativeDistance, relativeDistance, screenPos)=> {
        updateValue(relativeDistance, screenPos);
        handleRoot.view3D.changeObjectProperty([handleRoot.targetNode], [handleRoot.propName]);
    }

    onReleased: (mouseArea, sceneRelativeDistance, relativeDistance, screenPos)=> {
        updateValue(relativeDistance, screenPos);
        handleRoot.view3D.commitObjectProperty([handleRoot.targetNode], [handleRoot.propName]);
    }
}
