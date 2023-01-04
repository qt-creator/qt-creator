// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 6.0
import QtQuick3D 6.0

Node {
    id: armRoot
    property alias posModel: posModel
    property alias negModel: negModel
    property View3D view3D
    property color hoverColor
    property color color
    property vector3d camRotPos
    property vector3d camRotNeg

    Model {
        id: posModel

        property bool hovering: false
        property vector3d cameraRotation: armRoot.camRotPos

        source: "../meshes/axishelper.mesh"
        materials: DefaultMaterial {
            id: posMat
            diffuseColor: posModel.hovering ? armRoot.hoverColor : armRoot.color
            lighting: DefaultMaterial.NoLighting
        }
        pickable: true
    }

    Model {
        id: negModel

        property bool hovering: false
        property vector3d cameraRotation: armRoot.camRotNeg

        source: "#Sphere"
        y: -6
        scale: Qt.vector3d(0.025, 0.025, 0.025)
        materials: DefaultMaterial {
            id: negMat
            diffuseColor: negModel.hovering ? armRoot.hoverColor : armRoot.color
            lighting: DefaultMaterial.NoLighting
        }
        pickable: true
    }
}
