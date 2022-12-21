// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.9
import QtQuick.Shapes 1.0

Shape {
    id: root

    //Antialiasing workaround
    layer.enabled: true
    layer.textureSize: Qt.size(width * 2, height * 2)
    layer.smooth: true

    implicitWidth: 100
    implicitHeight: 100

    property alias fillColor: path.fillColor
    property alias strokeColor: path.strokeColor
    property alias strokeWidth: path.strokeWidth
    property alias capStyle: path.capStyle
    property alias strokeStyle: path.strokeStyle
    property alias fillGradient: path.fillGradient

    property real begin: 0
    property real end: 90

    property real alpha: end - begin

    function polarToCartesianX(centerX, centerY, radius, angleInDegrees) {
        var angleInRadians = angleInDegrees * Math.PI / 180.0
        var x = centerX + radius * Math.cos(angleInRadians)
        return x
    }

    function polarToCartesianY(centerX, centerY, radius, angleInDegrees) {
        var angleInRadians = angleInDegrees * Math.PI / 180.0
        var y = centerY + radius * Math.sin(angleInRadians)
        return y
    }

    ShapePath {
        id: path

        property real __xRadius: width / 2 - strokeWidth / 2
        property real __yRadius: height / 2 - strokeWidth / 2

        property real __Xcenter: width / 2
        property real __Ycenter: height / 2
        strokeColor: "#000000"

        fillColor: "transparent"
        strokeWidth: 1
        capStyle: ShapePath.FlatCap

        startX: root.polarToCartesianX(path.__Xcenter, path.__Ycenter,
                                       path.__xRadius, root.begin - 180)
        startY: root.polarToCartesianY(path.__Xcenter, path.__Ycenter,
                                       path.__yRadius, root.begin - 180)

        PathArc {
            id: arc

            x: root.polarToCartesianX(path.__Xcenter, path.__Ycenter,
                                      path.__xRadius, root.end - 180)
            y: root.polarToCartesianY(path.__Xcenter, path.__Ycenter,
                                      path.__yRadius, root.end - 180)

            radiusY: path.__yRadius
            radiusX: path.__xRadius

            useLargeArc: root.alpha > 180
        }
    }
}
