// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick 2.0

Item {
    id: rootItem
    property variant input
    property variant output
    property variant sourceRect
    visible: false

    Component.onCompleted: evaluateInput()

    onInputChanged: evaluateInput()

    onSourceRectChanged: evaluateInput()

    function evaluateInput() {
        if (input == undefined) {
            output =  input
        }
        else if (sourceRect != undefined && sourceRect != Qt.rect(0, 0, 0, 0) && !isQQuickShaderEffectSource(input)) {
            proxySource.sourceItem = input
            output = proxySource
            proxySource.sourceRect = sourceRect
        }
        else if (isQQuickItemLayerEnabled(input)) {
            output = input
        }
        else if ((isQQuickImage(input) && !hasTileMode(input) && !hasChildren(input))) {
            output = input
        }
        else if (isQQuickShaderEffectSource(input)) {
            output = input
        }
        else {
            proxySource.sourceItem = input
            output = proxySource
            proxySource.sourceRect = Qt.rect(0, 0, 0, 0)
        }
    }

    function isQQuickItemLayerEnabled(item) {
        if (item.hasOwnProperty("layer")) {
            var l = item["layer"]
            if (l.hasOwnProperty("enabled") && l["enabled"].toString() == "true")
                return true
        }
        return false
    }

    function isQQuickImage(item) {
        var imageProperties = [ "fillMode", "progress", "asynchronous", "sourceSize", "status", "smooth" ]
        return hasProperties(item, imageProperties)
    }

    function isQQuickShaderEffectSource(item) {
        var shaderEffectSourceProperties = [ "hideSource", "format", "sourceItem", "mipmap", "wrapMode", "live", "recursive", "sourceRect" ]
        return hasProperties(item, shaderEffectSourceProperties)
    }

    function hasProperties(item, properties) {
        var counter = 0
            for (var j = 0; j < properties.length; j++) {
                if (item.hasOwnProperty(properties [j]))
                    counter++
            }
        return properties.length == counter
    }

    function hasChildren(item) {
        if (item.hasOwnProperty("childrenRect")) {
            if (item["childrenRect"].toString() != "QRectF(0, 0, 0, 0)")
                return true
            else
                return false
        }
        return false
    }

    function hasTileMode(item) {
        if (item.hasOwnProperty("fillMode")) {
            if (item["fillMode"].toString() != "0")
                return true
            else
                return false
        }
        return false
    }

    ShaderEffectSource {
        id: proxySource
        live: rootItem.input != rootItem.output
        hideSource: false
        smooth: true
        visible: false
    }
}

