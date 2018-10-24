/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

import QtQuick 2.6
import QtQuickDesignerTheme 1.0

Item {

    id: root
    property alias color: background.color
    width: 110
    height: 110

    Rectangle {
        id: background
        color: Theme.qmlDesignerBackgroundColorDarker()
        anchors.fill: parent
        anchors.rightMargin: 4
        anchors.bottomMargin: 5
        anchors.leftMargin: 2
    }


    property bool interactive: true

    property alias bezierCurve: canvas.curve

    property alias caption: label.text

    property color handleColor: "#d5d251"
    property color bezierColor: Theme.color(Theme.PanelTextColorLight)
    property color textColor: Theme.color(Theme.PanelTextColorLight)

    property real bezierLineWidth: 2
    property real handleLineWidth: 1

    signal clicked

    Text {
        x: -7
        y: 3
        color: root.textColor
        text: qsTr("value")
        visible: root.interactive
        rotation: 90
        font.pixelSize: 5
    }

    Text {
        x: 105 - width
        y: 112 - height
        color: root.textColor
        text: qsTr("time")
        visible: root.interactive
        font.pixelSize: 5
    }

    Canvas {
        id: canvas

        width:110
        height:110
        property real bezierLineWidth: root.bezierLineWidth * 2
        property real handleLineWidth: root.handleLineWidth
        property color strokeStyle:  root.bezierColor
        property color secondaryStrokeStyle:  root.handleColor
        antialiasing: true
        onStrokeStyleChanged:requestPaint();

        property string curve: "0,0,1,1"

        function curveToBezier()
        {
            var res =  canvas.curve.split(",");

            var array = [res[0], res[1], res[2], res[3], 1, 1];
            return array;
        }

        Component.onCompleted: {
            var old = curve
            curve = ""
            curve = old
        }
        property bool block: false
        onCurveChanged: {
            textEdit.text = curve
            if (canvas.block)
                return

            var res =  canvas.curve.split(",");

            control1.x = res[0] * 100 - 5
            control1.y = (1 - res[1]) * 100 - 5
            control2.x = res[2] * 100 - 5
            control2.y = (1 - res[3]) * 100 - 5
        }

        function curveFromControls()
        {
            canvas.block = true
            canvas.curve = ((control1.x + 5) / 100).toFixed(2) +","+(1 - (control1.y + 5) / 100).toFixed(2) +","
                    + ((control2.x + 5) / 100).toFixed(2) +","+(1 - (control2.y + 5) / 100).toFixed(2)
            canvas.block = false
        }

        onPaint: {
            var ctx = canvas.getContext('2d');
            ctx.save();
            ctx.clearRect(0, 0, canvas.width, canvas.height);
            ctx.translate(4, 4);

            var res =  canvas.curve.split(",");

            ctx.lineWidth = canvas.handleLineWidth;

            ctx.strokeStyle = canvas.secondaryStrokeStyle;

            ctx.beginPath();
            ctx.moveTo(0,100);
            ctx.lineTo(control1.x+5,control1.y+5);
            ctx.stroke();

            ctx.beginPath();
            ctx.moveTo(100,0);
            ctx.lineTo(control2.x+5,control2.y+5);
            ctx.stroke();

            ctx.strokeStyle = canvas.strokeStyle;
            ctx.lineWidth = canvas.bezierLineWidth;

            ctx.beginPath();
            ctx.moveTo(0,100);
            ctx.bezierCurveTo(res[0]*100,(1-res[1])*100,res[2]*100,(1-res[3])*100, 100, 0);

            ctx.stroke();
            ctx.restore();
        }
    }
    MouseArea {
        //visible: !root.interactive
        anchors.fill: parent
        onClicked: root.clicked()
        onDoubleClicked: animation.start()
        SequentialAnimation {
            id: animation
            loops: 1
            running: false

            PropertyAction {
                target: animationIndicator
                property: "visible"
                value: true
            }
            ParallelAnimation {
                PropertyAnimation {
                    duration: 800
                    target: animationIndicator
                    property: "x"
                    from: -5
                    to: 95

                }

                PropertyAnimation {
                    duration: 800
                    target: animationIndicator
                    property: "y"
                    from: 95
                    to: -5
                    easing.bezierCurve: canvas.curveToBezier()
                }
            }
            PropertyAction {
                target: animationIndicator
                property: "visible"
                value: false
            }
        }
    }

    Item {
        width: 100
        height: 100
        x: 5
        y: 5

        Rectangle {
            visible: root.interactive
            id: control1
            color: root.handleColor
            width: 8
            height: 8
            radius: 4
            x: -4
            y: 96

            onXChanged: {
                if (mirrorShift)
                    control2.x = 96 - x - 4
                if (mirrorControl)
                    control2.x = y

                canvas.curveFromControls()
                canvas.requestPaint()
            }
            onYChanged: {
                if (mirrorShift)
                    control2.y = 96 - y - 4
                if (mirrorControl)
                    control2.y = x

                canvas.curveFromControls()
                canvas.requestPaint()
            }
            property bool mirrorShift: false
            property bool mirrorControl: false



            MouseArea {
                anchors.fill: parent

                drag.target: parent
                drag.maximumX: 95
                drag.maximumY: 95
                drag.minimumX: -5
                drag.minimumY: -5

                onPressed: {
                    if ((mouse.button == Qt.LeftButton) && (mouse.modifiers & Qt.ShiftModifier)) {
                        parent.mirrorShift = true
                    } else if ((mouse.button == Qt.LeftButton) && (mouse.modifiers & Qt.ControlModifier)) {
                        parent.mirrorControl = true
                    } else {
                        parent.mirrorShift = false
                    }
                }
                onReleased: {
                    parent.mirrorControl = false
                    parent.mirrorShift = false
                }
            }
        }

        Rectangle {
            color: root.handleColor
            width: radius * 2
            height: radius * 2
            radius: root.interactive ? 3 : 4
            y: -4
            x: 96
        }

        Rectangle {
            color: root.handleColor
            width: radius * 2
            height: radius * 2
            radius: root.interactive ? 3 : 4
            x: -4
            y: 96
        }


        Rectangle {
            visible: root.interactive
            id: control2
            color: root.handleColor
            width: 8
            height: 8
            radius: 4
            y: -5
            x: 5
            onXChanged: {
                if (mirrorShift)
                    control1.x = 96 - x - 4
                if (mirrorControl)
                    control1.x = y
                canvas.curveFromControls()
                canvas.requestPaint()
            }
            onYChanged: {
                if (mirrorShift)
                    control1.y = 96 - y -4
                if (mirrorControl)
                    control1.y = x
                canvas.curveFromControls()
                canvas.requestPaint()
            }
            property bool mirrorShift: false
            property bool mirrorControl: false


            MouseArea {
                anchors.fill: parent

                drag.target: parent
                drag.maximumX: 95
                drag.maximumY: 95
                drag.minimumX: -5
                drag.minimumY: -5
                onPressed: {
                    if ((mouse.button == Qt.LeftButton) && (mouse.modifiers & Qt.ShiftModifier)) {
                        parent.mirrorShift = true
                    } else if ((mouse.button == Qt.LeftButton) && (mouse.modifiers & Qt.ControlModifier)) {
                        parent.mirrorControl = true
                    } else {
                        parent.mirrorShift = false
                    }
                }
                onReleased: {
                    parent.mirrorControl = false
                    parent.mirrorShift = false
                }
            }
        }

        Rectangle {
            visible: animationIndicator.visible
            color: animationIndicator.color
            height: 1
            x: -2
            width: animationIndicator.x + 2
            y: animationIndicator.y + 2
        }

        Rectangle {
            visible: animationIndicator.visible
            color: animationIndicator.color
            height: 1
            x: animationIndicator.x - 2
            width: 120 - animationIndicator.x
            y: animationIndicator.y + 2
        }

        Rectangle {
            visible: animationIndicator.visible
            color: animationIndicator.color
            width: 1
            y: -2
            height: animationIndicator.y + 2
            x: animationIndicator.x + 2
        }

        Rectangle {
            visible: animationIndicator.visible
            color: animationIndicator.color
            width: 1
            y: animationIndicator.y - 2
            height: 110 - animationIndicator.y
            x: animationIndicator.x + 2
        }

        Rectangle {
            x: 5
            y: 5
            id: animationIndicator
            visible: false
            color: Theme.color(Theme.QmlDesigner_HighlightColor)
            width: 4
            height: 4
            radius: 2
        }
    }

    TextInput {
        visible: false
        id: textEdit

        height: 24
        anchors.right: parent.right
        anchors.left: parent.left
        anchors.top: parent.bottom
        color: root.textColor
        horizontalAlignment: Text.AlignHCenter
        anchors.topMargin: -20
        onEditingFinished: canvas.curve = text
    }

    Text {
        id: label
        visible: !root.interactive
        height: 24
        anchors.right: parent.right
        anchors.left: parent.left
        anchors.top: parent.bottom
        color: root.textColor
        horizontalAlignment: Text.AlignHCenter
        anchors.topMargin: -2
        font.pixelSize: 16

    }

}
