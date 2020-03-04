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

import QtQuick 2.0
import QtQuick3D 1.15

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

        source: "meshes/axishelper.mesh"
        materials: DefaultMaterial {
            id: posMat
            emissiveColor: posModel.hovering ? armRoot.hoverColor : armRoot.color
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
            emissiveColor: negModel.hovering ? armRoot.hoverColor : armRoot.color
            lighting: DefaultMaterial.NoLighting
        }
        pickable: true
    }
}
