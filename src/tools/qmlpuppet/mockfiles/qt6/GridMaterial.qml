// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick3D

CustomMaterial {
    property real alphaStartDepth: 500
    property real alphaEndDepth: 40000
    property real generalAlpha: 1
    property color color: "#000000"
    property real density: 50
    property bool orthoMode: false

    vertexShader: Qt.resolvedUrl("../shaders/gridmaterial.vert")
    fragmentShader: Qt.resolvedUrl("../shaders/gridmaterial.frag")
    sourceBlend: CustomMaterial.NoBlend
    destinationBlend: CustomMaterial.NoBlend
    shadingMode: CustomMaterial.Unshaded
    cullMode: Material.NoCulling
}
