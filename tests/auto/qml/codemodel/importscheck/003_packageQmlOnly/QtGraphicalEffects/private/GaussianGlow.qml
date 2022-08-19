// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick 2.0

Item {
    id: rootItem
    property variant source
    property real radius: 0.0
    property int maximumRadius: 0
    property real spread: 0.0
    property color color: "white"
    property bool cached: false
    property bool transparentBorder: false

    SourceProxy {
        id: sourceProxy
        input: rootItem.source
        sourceRect: rootItem.transparentBorder ? Qt.rect(-1, -1, parent.width + 2.0, parent.height + 2.0) : Qt.rect(0, 0, 0, 0)
    }

    ShaderEffectSource {
        id: cacheItem
        anchors.fill: shaderItem
        visible: rootItem.cached
        smooth: true
        sourceItem: shaderItem
        live: true
        hideSource: visible
    }

    GaussianDirectionalBlur {
        id: shaderItem
        x: transparentBorder ? -maximumRadius - 1 : 0
        y: transparentBorder ? -maximumRadius - 1 : 0
        width: horizontalBlur.width
        height: horizontalBlur.height
        horizontalStep: 0.0
        verticalStep: 1.0 / parent.height
        source: horizontalBlur
        radius: rootItem.radius
        maximumRadius: rootItem.maximumRadius
        transparentBorder: rootItem.transparentBorder
        enableColor: true
        color: rootItem.color
        spread: rootItem.spread
    }

    GaussianDirectionalBlur {
        id: horizontalBlur
        width: transparentBorder ? parent.width + 2 * maximumRadius + 2 : parent.width
        height: transparentBorder ? parent.height + 2 * maximumRadius + 2  : parent.height
        horizontalStep: 1.0 / parent.width
        verticalStep: 0.0
        source: sourceProxy.output
        radius: rootItem.radius
        maximumRadius: rootItem.maximumRadius
        transparentBorder: rootItem.transparentBorder
        visible: false
    }
}
