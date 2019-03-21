/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

import QtQuick 2.1
import HelperWidgets 2.0
import QtQuick.Controls.Private 1.0 // showing a ToolTip

Item {
    width: 300
    height: 60

    property color currentColor
    property alias model: repeater.model

    property bool hasGradient: gradientModel.hasGradient


    property alias gradientPropertyName: gradientModel.gradientPropertyName
    property alias gradientTypeName: gradientModel.gradientTypeName

    onHasGradientChanged: {
        colorLine.invalidate()
    }

    onCurrentColorChanged: {
        gradientModel.setColor(colorLine.selectedIndex, currentColor)
        colorLine.invalidate()
    }

    function addGradient() {
        gradientModel.addGradient()
        colorLine.invalidate()
        colorLine.select(0)
    }

    function deleteGradient() {
        gradientModel.deleteGradient()
    }

    Connections {
        target: modelNodeBackend
        onSelectionChanged: {
            colorLine.invalidate()
            colorLine.select(0)
        }
    }

    Item {
        id: colorLine
        height: 80
        width: parent.width

        property int effectiveWidth: width - 10
        property int selectedIndex: 0

        function select(index) {
            for (var i = 0; i < repeater.model.count; i++) {
                repeater.itemAt(i).item.highlighted = false
            }

            if (repeater.model.count < index + 1)
                return;

            repeater.itemAt(index).item.highlighted = true
            colorLine.selectedIndex = index

            gradientModel.lock()
            currentColor = repeater.itemAt(index).item.color
            gradientModel.unlock()
        }

        function invalidate() {
            var gradientString = "import QtQuick 2.0; Gradient {"

            for (var i = 0; i < gradientModel.count; i++) {
                gradientString += "GradientStop {}"
            }
            gradientString += "}"

            var gradientObject = Qt.createQmlObject(gradientString, gradientRectangle, "test");

            for (i = 0; i < gradientModel.count; i++) {
                if (repeater.itemAt(i) !== null)
                    repeater.itemAt(i).item.y = 20 //fixes corner case for dragging overlapped items

                gradientObject.stops[i].color =  gradientModel.getColor(i)
                gradientObject.stops[i].position = gradientModel.getPosition(i)
            }

            gradientRectangle.gradient = gradientObject;
        }

        Column {
            anchors.fill: parent
            MouseArea {
                height: 40
                anchors.left: parent.left
                anchors.right: parent.right

                onClicked: {
                    var currentPosition = mouseX / colorLine.effectiveWidth

                    var newIndex = gradientModel.addStop(currentPosition, currentColor)

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
                    model: GradientModel {
                        anchorBackendProperty: anchorBackend
                        gradientPropertyName: "gradient"
                        id: gradientModel

                    }

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
                colorLine.select(0);
                colorLine.invalidate();
            }

            Item {
                height: 16
                anchors.left: parent.left
                anchors.right: parent.right
                Rectangle {
                    smooth: true
                    x: 0
                    y: 16
                    radius: 1
                    border.color: "#555555"
                    border.width: 1
                    width: parent.height
                    height: parent.width
                    id: gradientRectangle
                    gradient: Gradient {
                        id: gradient
                    }
                    transformOrigin: Item.TopLeft
                    rotation: 270
                }
            }
        }
    }

    Component {
        id: component
        Item {
            id: gradientStopHandle
            y: 20
            width: 10
            height: 20

            property alias color: rectangle.color
            property alias highlighted: canvas.highlighted

            property bool toolTipVisible: false

            function refreshToolTip(showToolTip) {
                toolTipVisible = showToolTip;
                if (showToolTip) {
                    var currentPoint = Qt.point(gradientStopHandleMouseArea.mouseX, gradientStopHandleMouseArea.mouseY);
                    var fixedGradiantStopPosition = currentGradiantStopPosition();
                    Tooltip.showText(gradientStopHandleMouseArea, currentPoint, fixedGradiantStopPosition.toFixed(3));
                } else {
                    Tooltip.hideText()
                }
            }
            function currentGradiantStopPosition() {
                return x / colorLine.effectiveWidth;
            }

            onXChanged: {
                refreshToolTip(toolTipVisible)
            }

            Rectangle {
                id: rectangle
                width: 10
                height: 10
                color: "red"
                border.color: "gray"
                border.width: 1
                radius: 1

            }

            Canvas {
                id: canvas
                width: 10
                height: 10
                y: 10

                antialiasing: true

                property bool highlighted: false

                property color strokeStyle:  Qt.darker(fillStyle, 1.6)
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

            property bool readOnly: false
            property int index: 0

            opacity: y < 20 ? y / 10 : 1

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
                drag.minimumY: !readOnly ? 0 : 20
                drag.maximumY: 20

                // using pressed property instead of drag.active which was not working
                onExited: {
                    gradientStopHandle.refreshToolTip(pressed);
                }
                onCanceled: {
                    gradientStopHandle.refreshToolTip(pressed);
                }
                hoverEnabled: true

                Timer {
                    interval: 1000
                    running: gradientStopHandleMouseArea.containsMouse
                    onTriggered: {
                        gradientStopHandle.refreshToolTip(true);
                    }
                }

                onPressed: {
                    colorLine.select(index);
                    gradientStopHandle.refreshToolTip(true);
                }

                onReleased: {
                    if (drag.active) {
                        gradientModel.setPosition(colorLine.selectedIndex, gradientStopHandle.currentGradiantStopPosition())
                        gradientStopHandle.refreshToolTip(false);

                        if (parent.y < 10) {
                            if (!readOnly) {
                                colorLine.select(index - 1)
                                gradientModel.removeStop(index)
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
