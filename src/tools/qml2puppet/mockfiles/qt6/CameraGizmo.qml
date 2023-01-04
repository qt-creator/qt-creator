// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 6.0
import QtQuick3D 6.0

IconGizmo {
    id: cameraGizmo

    property Model frustumModel: null
    property bool globalShowFrustum: false

    iconSource: "qrc:///qtquickplugin/mockfiles/images/editor_camera.png"

    function connectFrustum(frustum)
    {
        frustumModel = frustum;

        frustum.selected = selected;
        frustum.selected = Qt.binding(function() {return selected;});

        frustum.scene = scene;
        frustum.scene = Qt.binding(function() {return scene;});

        frustum.targetNode = targetNode;
        frustum.targetNode = Qt.binding(function() {return targetNode;});

        frustum.visible = (canBeVisible && globalShowFrustum)
                          || (targetNode && selected && activeScene === scene);
        frustum.visible = Qt.binding(function() {
            return (canBeVisible && globalShowFrustum)
                   || (targetNode && selected && activeScene === scene);
        });
    }

    onActiveSceneChanged: {
        if (frustumModel && activeScene == scene)
            frustumModel.updateGeometry();
    }
}
