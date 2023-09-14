// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 6.0
import QtQuick3D 6.0
import SelectionBoxGeometry 1.0

Node {
    id: selectionBox

    property View3D view3D
    property Node targetNode: null
    property alias model: selectionBoxModel
    property alias geometryName: selectionBoxGeometry.name
    property bool showBox: true

    SelectionBoxGeometry {
        id: selectionBoxGeometry
        name: "Selection Box of 3D Edit View"
        view3D: selectionBox.view3D
        targetNode: selectionBox.targetNode
        rootNode: selectionBox
    }

    Model {
        id: selectionBoxModel
        readonly property bool _edit3dLocked: true // Make this non-pickable
        geometry: selectionBoxGeometry

        scale: selectionBox.targetNode ? selectionBox.targetNode.scale : Qt.vector3d(1, 1, 1)
        rotation: selectionBox.targetNode ? selectionBox.targetNode.rotation : Qt.quaternion(1, 0, 0, 0)
        position: selectionBox.targetNode ? selectionBox.targetNode.position : Qt.vector3d(0, 0, 0)
        pivot: selectionBox.targetNode ? selectionBox.targetNode.pivot : Qt.vector3d(0, 0, 0)

        visible: selectionBox.targetNode

        castsShadows: false
        receivesShadows: false

        DefaultMaterial {
            id: boxMaterial
            diffuseColor: "#fff600"
            lighting: DefaultMaterial.NoLighting
            cullMode: Material.NoCulling
        }
        // We custom set the render node transform for the selectionBox node to correspond
        // targetNode's parent transform when we calculate the box geometry.
        // This transform gets reset if visibility of the box changes, so instead we hide the box
        // by setting null material to it.
        materials: selectionBox.showBox && !selectionBoxGeometry.isEmpty ? boxMaterial : null
    }
}
