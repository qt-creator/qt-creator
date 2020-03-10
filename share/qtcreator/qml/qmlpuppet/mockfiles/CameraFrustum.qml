/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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
import CameraGeometry 1.0

Model {
    id: cameraFrustum

    property alias geometryName: cameraGeometry.name // Name must be unique for each geometry
    property alias viewPortRect: cameraGeometry.viewPortRect
    property Node targetNode: null
    property Node scene: null
    property bool selected: false

    function updateGeometry()
    {
        cameraGeometry.update();
    }

    position: targetNode ? targetNode.scenePosition : Qt.vector3d(0, 0, 0)
    rotation: targetNode ? targetNode.sceneRotation : Qt.quaternion(1, 0, 0, 0)

    geometry: cameraGeometry
    materials: [
        DefaultMaterial {
            id: defaultMaterial
            emissiveColor: cameraFrustum.selected ? "#FF0000" : "#555555"
            lighting: DefaultMaterial.NoLighting
            cullMode: Material.NoCulling
        }
    ]

    CameraGeometry {
        id: cameraGeometry
        camera: cameraFrustum.scene && cameraFrustum.targetNode ? cameraFrustum.targetNode : null
    }
}
