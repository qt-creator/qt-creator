// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.0
import QtQuick3D 1.15
import LineGeometry 1.0

DirectionalDraggable {
    id: arrowRoot

    Model {
        geometry: LineGeometry {
            id: lineGeometry
            name: "Edit 3D ScalableArrow"
            startPos: Qt.vector3d(0, 0, 0)
            endPos: Qt.vector3d(0, 1, 0)
        }
        scale: Qt.vector3d(1, arrowRoot.length, 1)
        materials: [ arrowRoot.material ]
    }

    Model {
        id: arrowHead
        source: "#Cone"
        materials: [ arrowRoot.material ]
        y: arrowRoot.length - 3
        scale: Qt.vector3d(0.02, 0.035, 0.02)
    }
}
