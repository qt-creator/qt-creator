// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.0
import QtQuick3D 1.15
import LightUtils 1.0

Model {
    id: lightModel

    property alias geometryName: lightGeometry.name // Name must be unique for each geometry
    property alias geometryType: lightGeometry.lightType
    property Material material

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
