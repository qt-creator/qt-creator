// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick3D
import BoxGeometry

Model {
    id: reflectionProbeGizmo

    property Node selectedNode

    readonly property bool _edit3dLocked: true // Make this non-pickable
    readonly property bool _isProbe: selectedNode instanceof ReflectionProbe

    visible: _isProbe
    scale: _isProbe ? Qt.vector3d(selectedNode.boxSize.x,
                                 selectedNode.boxSize.y,
                                 selectedNode.boxSize.z)
                   : Qt.vector3d(1, 1, 1)

    geometry: BoxGeometry {}

    castsShadows: false
    receivesShadows: false
    castsReflections: false

    position: _isProbe ? selectedNode.scenePosition.plus(selectedNode.boxOffset)
                      : Qt.vector3d(0, 0, 0)

    materials: boxMaterial

    PrincipledMaterial {
        id: boxMaterial
        baseColor: "#03fcdb"
        lighting: DefaultMaterial.NoLighting
        cullMode: Material.NoCulling
    }
}
