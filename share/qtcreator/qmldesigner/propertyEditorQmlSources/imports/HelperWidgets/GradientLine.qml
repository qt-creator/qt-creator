// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import HelperWidgets 2.0
import StudioTheme 1.0 as StudioTheme

Item {
    id: root

    width: 300
    height: StudioTheme.Values.colorEditorPopupLineHeight

    property color currentColor
    property GradientModel model
    property GradientModel gradientModel: root.model

    property bool hasGradient: root.model.hasGradient

    signal selectedNodeChanged
    signal invalidated

    onHasGradientChanged: {
        colorLine.invalidate()
    }

    onCurrentColorChanged: {
        root.gradientModel.setColor(colorLine.selectedIndex, root.currentColor)
        colorLine.invalidate()
    }

    function addGradient() {
        root.gradientModel.addGradient()
        colorLine.invalidate()
        colorLine.select(0)
    }

    function deleteGradient() {
        root.gradientModel.deleteGradient()
    }

    function setPresetByID(presetID) {
        root.gradientModel.setPresetByID(presetID)
        colorLine.invalidate()
        colorLine.select(0)
    }

    function setPresetByStops(stopsPositions, stopsColors, stopsCount) {
        root.gradientModel.setPresetByStops(stopsPositions, stopsColors, stopsCount)
        colorLine.invalidate()
        colorLine.select(0)
    }

    function savePreset() {
        root.gradientModel.savePreset()
    }

    function updateGradient() {
        root.gradientModel.updateGradient()
    }

    Connections {
        target: modelNodeBackend
        function onSelectionChanged() {
            colorLine.invalidate()
            colorLine.select(0)
        }
    }

    Item {
        id: colorLine
        height: 80
        width: parent.width

        property int effectiveWidth: colorLine.width - 10
        property int selectedIndex: 0

        function select(index) {
            for (var i = 0; i < repeater.model.count; i++)
                repeater.itemAt(i).item.highlighted = false

            if (repeater.model.count < index + 1)
                return

            repeater.itemAt(index).item.highlighted = true
            colorLine.selectedIndex = index

            root.gradientModel.lock()
            root.currentColor = repeater.itemAt(index).item.color
            root.gradientModel.unlock()

            root.selectedNodeChanged()
        }

        function invalidate() {
            var gradientString = "import QtQuick 2.15; Gradient { orientation: Gradient.Horizontal;"

            for (var i = 0; i < root.gradientModel.count; i++) {
                gradientString += "GradientStop {}"
            }
            gradientString += "}"

            var gradientObject = Qt.createQmlObject(gradientString, gradientRectangle, "test")

            for (i = 0; i < root.gradientModel.count; i++) {
                if (repeater.itemAt(i) !== null)
                    repeater.itemAt(i).item.y = 20 // fixes corner case for dragging overlapped items

                gradientObject.stops[i].color = root.gradientModel.getColor(i)
                gradientObject.stops[i].position = root.gradientModel.getPosition(i)
            }

            gradientRectangle.gradient = gradientObject

            root.invalidated()
        }

        Column {
            anchors.fill: parent

            MouseArea {
                height: 40
                anchors.left: parent.left
                anchors.right: parent.right
                cursorShape: Qt.PointingHandCursor

                onClicked: {
                    var currentPosition = mouseX / colorLine.effectiveWidth
                    var newIndex = root.gradientModel.addStop(currentPosition, root.currentColor)

                    if (newIndex > 0)
                        colorLine.select(newIndex)

                    colorLine.invalidate()
                }

                Item {
                    id: stack
                    anchors.fill: parent
                }

                Repeater {
                    id: repeater
                    model: root.gradientModel

                    delegate: Loader {
                        id: loader

                        sourceComponent: component
                        Binding {
                            target: loader.item
                            property: "color"
                            value: color
                        }
                        Binding {
                            target: loader.item
                            property: "x"
                            value: position === undefined ? 0 : colorLine.effectiveWidth * position
                        }
                        Binding {
                            target: loader.item
                            property: "index"
                            value: index
                        }
                        Binding {
                            target: loader.item
                            property: "readOnly"
                            value: readOnly !== undefined ? readOnly : false
                        }
                    }
                }
            }

            Component.onCompleted: {
                colorLine.select(0)
                colorLine.invalidate()
            }

            Item {
                height: 16
                anchors.left: parent.left
                anchors.right: parent.right

                Image {
                    id: checkerboard
                    anchors.fill: parent
                    source: "images/checkers.png"
                    fillMode: Image.Tile
                }

                Rectangle {
                    id: gradientRectangle
                    anchors.fill: parent
                    border.color: StudioTheme.Values.themeControlOutline
                    border.width: StudioTheme.Values.border
                    gradient: Gradient {
                        id: gradient
                    }
                }
            }
        }
    }

    Tooltip {
        id: myTooltip
    }

    Component {
        id: component

        Item {
            id: gradientStopHandle

            property alias color: rectangle.color
            property alias highlighted: canvas.highlighted
            property bool toolTipVisible: false
            property bool readOnly: false
            property int index: 0

            y: 20
            width: 10
            height: 20
            opacity: y < 20 ? y / 10 : 1

            function refreshToolTip(showToolTip) {
                gradientStopHandle.toolTipVisible = showToolTip
                if (showToolTip) {
                    var currentPoint = Qt.point(gradientStopHandleMouseArea.mouseX,
                                                gradientStopHandleMouseArea.mouseY)
                    var fixedGradiantStopPosition = currentGradiantStopPosition()
                    myTooltip.showText(gradientStopHandleMouseArea,
                                       currentPoint,
                                       fixedGradiantStopPosition.toFixed(3))
                } else {
                    myTooltip.hideText()
                }
            }

            function currentGradiantStopPosition() {
                return gradientStopHandle.x / colorLine.effectiveWidth;
            }

            onXChanged: gradientStopHandle.refreshToolTip(gradientStopHandle.toolTipVisible)

            Image {
                width: 10
                height: 10
                source: "images/checkers.png"
                fillMode: Image.Tile
            }

            Rectangle {
                id: rectangle
                width: 10
                height: 10
                color: "red"
                border.color: "gray"
                border.width: 1
            }

            Canvas {
                id: canvas
                width: 10
                height: 10
                y: 10

                antialiasing: true

                property bool highlighted: false

                property color strokeStyle: Qt.darker(fillStyle, 1.6)
                property color fillStyle: highlighted ? "lightGray" : "gray"

                onHighlightedChanged: requestPaint()

                onPaint: {
                    var ctx = canvas.getContext('2d')
                    ctx.save()

                    ctx.strokeStyle = canvas.strokeStyle
                    ctx.lineWidth = 1
                    ctx.fillStyle = canvas.fillStyle

                    var size = 10
                    var yOffset = 2

                    ctx.beginPath()
                    ctx.moveTo(0, yOffset)
                    ctx.lineTo(size, yOffset)
                    ctx.lineTo(size / 2, 5 + yOffset)
                    ctx.closePath()
                    ctx.fill()
                    ctx.stroke()

                    ctx.restore()
                }
            }

            Behavior on y {
                PropertyAnimation {
                    duration: 100
                }
            }

            Behavior on opacity {
                PropertyAnimation {
                    duration: 100
                }
            }

            MouseArea {
                id: gradientStopHandleMouseArea
                anchors.fill: parent
                drag.target: parent
                drag.minimumX: 0
                drag.maximumX: colorLine.effectiveWidth
                drag.minimumY: !gradientStopHandle.readOnly ? 0 : 20
                drag.maximumY: 20
                cursorShape: Qt.PointingHandCursor
                hoverEnabled: true

                // using pressed property instead of drag.active which was not working
                onExited: gradientStopHandle.refreshToolTip(pressed)
                onCanceled: gradientStopHandle.refreshToolTip(pressed)

                Timer {
                    interval: 1000
                    running: gradientStopHandleMouseArea.containsMouse
                    onTriggered: gradientStopHandle.refreshToolTip(true)
                }

                onPressed: {
                    colorLine.select(index)
                    gradientStopHandle.refreshToolTip(true)
                }

                onReleased: {
                    if (drag.active) {
                        root.gradientModel.setPosition(colorLine.selectedIndex,
                                                  gradientStopHandle.currentGradiantStopPosition())
                        gradientStopHandle.refreshToolTip(false)

                        if (parent.y < 10) {
                            if (!gradientStopHandle.readOnly) {
                                colorLine.select(index - 1)
                                root.gradientModel.removeStop(index)
                            }
                        }
                        parent.y = 20
                        colorLine.invalidate()
                        colorLine.select(colorLine.selectedIndex)
                    }
                }
            }
        }
    }
}
