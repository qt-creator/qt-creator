// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.0

Item {
    id: root
    default property alias content: stack.children
    property alias maskBlurRadius: maskedBlur.radius
    property alias maskBlurSamples: maskedBlur.samples

    Row {
        visible: true
        id: stack
    }

    Item {
        id: mask
        width: stack.width
        height: stack.height
        visible: false
    }

    ShaderEffectSource {
        id: maskedBlur
        anchors.fill: stack
        sourceItem: stack
        property real radius: 0
        property real samples: 0
    }
}
