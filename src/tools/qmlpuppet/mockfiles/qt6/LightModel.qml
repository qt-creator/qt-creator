// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 6.0
import QtQuick3D 6.0
import LightUtils 1.0

Model {
    id: lightModel

    property alias geometryName: lightGeometry.name // Name must be unique for each geometry
    property alias geometryType: lightGeometry.lightType
    property Material material
    readonly property bool _edit3dLocked: true // Make this non-pickable

    function updateGeometry()
    {
        lightGeometry.update();
    }

    scale: Qt.vector3d(50, 50, 50)

    geometry: lightGeometry
    materials: [ material ]

    LightGeometry {
        id: lightGeometry
    }
}
