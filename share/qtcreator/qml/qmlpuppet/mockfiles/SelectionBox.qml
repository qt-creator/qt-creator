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
import SelectionBoxGeometry 1.0

Node {
    id: selectionBox

    property View3D view3D
    property Node targetNode: null
    property alias model: selectionBoxModel
    property alias geometryName: selectionBoxGeometry.name

    SelectionBoxGeometry {
        id: selectionBoxGeometry
        name: "Selection Box of 3D Edit View"
        view3D: selectionBox.view3D
        targetNode: selectionBox.targetNode
        rootNode: selectionBox
    }

    Model {
        id: selectionBoxModel
        geometry: selectionBoxGeometry

        scale: selectionBox.targetNode ? selectionBox.targetNode.scale : Qt.vector3d(1, 1, 1)
        rotation: selectionBox.targetNode ? selectionBox.targetNode.rotation : Qt.quaternion(1, 0, 0, 0)
        position: selectionBox.targetNode ? selectionBox.targetNode.position : Qt.vector3d(0, 0, 0)
        pivot: selectionBox.targetNode ? selectionBox.targetNode.pivot : Qt.vector3d(0, 0, 0)

        visible: selectionBox.targetNode && !selectionBoxGeometry.isEmpty

        materials: [
            DefaultMaterial {
                emissiveColor: "#fff600"
                lighting: DefaultMaterial.NoLighting
                cullMode: Material.NoCulling
            }
        ]
    }
}
