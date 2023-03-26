// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.9
import QtQuick.Shapes 1.0

Shape {
    id: root

    implicitWidth: 100
    implicitHeight: 100

    property alias gradient: path.fillGradient
    property alias strokeStyle: path.strokeStyle
    property alias strokeWidth: path.strokeWidth
    property alias strokeColor: path.strokeColor
    property alias dashPattern: path.dashPattern
    property alias joinStyle: path.joinStyle
    property alias fillColor: path.fillColor
    property alias capStyle: path.capStyle
    property alias dashOffset: path.dashOffset

    property real begin: 0
    property real end: 90

    property real arcWidth: 10

    property real arcWidthBegin: arcWidth
    property real arcWidthEnd: arcWidth

    property real radiusInnerAdjust: 0
    property real radiusOuterAdjust: 0

    property real alpha: clamp(sortedEnd() - sortedBegin(),0, 359.9)

    layer.enabled: antialiasing
    layer.smooth: antialiasing
    layer.textureSize: Qt.size(width * 2, height * 2)
    property bool outlineArc: false

    property bool round: false

    property bool roundEnd: round
    property bool roundBegin: round

    function clamp(num, min, max) {
        return num <= min ? min : num >= max ? max : num;
    }

    function myCos(angleInDegrees) {
        var angleInRadians = angleInDegrees * Math.PI / 180.0;
        return Math.cos(angleInRadians)
    }

    function mySin(angleInDegrees) {
        var angleInRadians = angleInDegrees * Math.PI / 180.0;
        return Math.sin(angleInRadians)
    }

    function polarToCartesianX(centerX, centerY, radius, angleInDegrees) {
        var angleInRadians = angleInDegrees * Math.PI / 180.0;
        var x = centerX + radius * Math.cos(angleInRadians)
        return x
    }

    function polarToCartesianY(centerX, centerY, radius, angleInDegrees) {
        var angleInRadians = angleInDegrees * Math.PI / 180.0;
        var y = centerY + radius * Math.sin(angleInRadians);
        return y
    }

    function calc()
    {
        path.__xRadius = root.width / 2 - root.strokeWidth / 2
        path.__yRadius = root.height / 2 - root.strokeWidth / 2

        path.__Xcenter = root.width / 2
        path.__Ycenter = root.height / 2

        path.startX = root.polarToCartesianX(path.__Xcenter, path.__Ycenter, path.__xRadius, root.sortedBegin() - 90) +  root.__beginOff * myCos(root.sortedBegin() + 90)
        path.startY = root.polarToCartesianY(path.__Xcenter, path.__Ycenter, path.__yRadius, root.sortedBegin() - 90) + root.__beginOff * mySin(root.sortedBegin() + 90)

        arc1.x = root.polarToCartesianX(path.__Xcenter, path.__Ycenter, path.__xRadius, root.sortedEnd() - 90) + root.__endOff * myCos(root.sortedEnd() + 90)
        arc1.y = root.polarToCartesianY(path.__Xcenter, path.__Ycenter,  path.__yRadius, root.sortedEnd() - 90) + root.__endOff * mySin(root.sortedEnd() + 90)

        arc1.radiusX =  path.__xRadius - root.__endOff / 2 -root.__beginOff / 2 + root.radiusOuterAdjust
        arc1.radiusY =  path.__yRadius - root.__endOff / 2 -root.__beginOff / 2 + root.radiusOuterAdjust

        arc1.useLargeArc =  root.alpha > 180
    }

    function sortedBegin()
    {
        return(Math.min(root.begin, root.end))
    }

    function sortedEnd()
    {
        return(Math.max(root.begin, root.end))
    }


    onWidthChanged: calc()
    onHeightChanged: calc()
    onBeginChanged: calc()
    onEndChanged: calc()
    onAlphaChanged: calc()

    ShapePath {
        id: path

        property real __xRadius
        property real __yRadius

        property real __Xcenter
        property real __Ycenter

        strokeColor: "red"
        strokeWidth: 4
        capStyle: ShapePath.FlatCap
    }

    property real __beginOff: {

        if (root.arcWidthEnd > root.arcWidthBegin)
            return (root.arcWidthEnd - root.arcWidthBegin) / 2

        return 0;
    }

    property real __endOff: {

        if (root.arcWidthBegin > root.arcWidthEnd)
            return (root.arcWidthBegin - root.arcWidthEnd) / 2

        return 0;
    }

    property real __startP: root.arcWidthBegin + __beginOff
    property real __endP: root.arcWidthEnd + __endOff

    Item {
        id: shapes
        PathArc {
            id: arc1
            property bool add: true
        }

        PathLine {
            relativeX: root.arcWidthEnd * myCos(root.sortedEnd())
            relativeY: root.arcWidthEnd * mySin(root.sortedEnd())
            property bool add: !root.roundEnd && (root.outlineArc && root.alpha < 359.8)

        }

        PathArc {
            relativeX: root.arcWidthEnd * myCos(root.sortedEnd())
            relativeY: root.arcWidthEnd * mySin(root.sortedEnd())
            radiusX: root.arcWidthEnd /2
            radiusY: root.arcWidthEnd /2
            property bool add: root.roundEnd && (root.outlineArc && root.alpha < 359.8)
        }

        PathMove {
            relativeX: root.arcWidthEnd * myCos(root.sortedEnd())
            relativeY: root.arcWidthEnd * mySin(root.sortedEnd())
            property bool add: root.outlineArc && root.alpha > 359.7
        }

        PathArc {
            id: arc2
            useLargeArc: arc1.useLargeArc

            radiusX: path.__xRadius - root.arcWidthBegin + root.__beginOff / 2 + root.__endOff / 2  + root.radiusInnerAdjust
            radiusY:path.__yRadius - root.arcWidthBegin + root.__beginOff / 2 + root.__endOff / 2 + root.radiusInnerAdjust

            x: path.startX + root.arcWidthBegin * myCos(root.sortedBegin())
            y: path.startY + root.arcWidthBegin * mySin(root.sortedBegin())

            direction: PathArc.Counterclockwise

            property bool add: root.outlineArc
        }


        PathLine {
            x: path.startX
            y: path.startY
            property bool add: !root.roundBegin && root.outlineArc && root.alpha < 359.8

        }

        PathArc {
            x: path.startX
            y: path.startY
            radiusX: root.arcWidthEnd /2
            radiusY: root.arcWidthEnd /2
            property bool add: root.roundBegin && root.outlineArc && root.alpha < 359.8
        }

        PathMove {
            x: path.startX
            y: path.startY
            property bool add: root.outlineArc && root.alpha == 360
        }
    }

    function invalidatePaths() {
        if (!root.__completed)
            return

        for (var i = 0; i < shapes.resources.length; i++) {
            var s = shapes.resources[i];
            if (s.add)
                path.pathElements.push(s)
        }

    }

    property bool __completed: false

    Component.onCompleted: {
        root.__completed = true
        invalidatePaths()
        calc()
    }
}
