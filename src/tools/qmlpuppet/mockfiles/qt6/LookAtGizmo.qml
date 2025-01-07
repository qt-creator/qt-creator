// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick3D
import LookAtGeometry

Node {
    id: root

    property alias crossScale: lookAtGeometry.crossScale
    property alias color: lookAtMat.baseColor

    Model {
        readonly property bool _edit3dLocked: true // Make this non-pickable
        geometry: LookAtGeometry {
            id: lookAtGeometry
        }
        materials: [
            PrincipledMaterial {
                id: lookAtMat
                lighting: DefaultMaterial.NoLighting
                cullMode: Material.NoCulling
            }
        ]
    }
}
