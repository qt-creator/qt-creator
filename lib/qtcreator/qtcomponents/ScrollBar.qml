/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

import QtQuick 1.0
import "custom" as Components

Item {
    id: scrollbar

    property int orientation : Qt.Horizontal
    property alias minimumValue: slider.minimumValue
    property alias maximumValue: slider.maximumValue
    property int pageStep: styleitem.horizontal ? width : height
    property int singleStep: 20
    property alias value: slider.value
    property bool scrollToClickposition: styleitem.styleHint("scrollToClickPosition")

    width: orientation == Qt.Horizontal ? 200 : internal.scrollbarExtent
    height: orientation == Qt.Horizontal ? internal.scrollbarExtent : 200

    onValueChanged: internal.updateHandle()

    MouseArea {
        id: internal

        anchors.fill: parent
        property bool upPressed
        property bool downPressed
        property bool pageUpPressed
        property bool pageDownPressed

        property bool autoincrement: false
        property int scrollbarExtent : styleitem.pixelMetric("scrollbarExtent");
        property bool handlePressed

        // Update hover item
        onEntered: styleitem.activeControl = styleitem.hitTest(mouseX, mouseY)
        onExited: styleitem.activeControl = "none"
        onMouseXChanged: styleitem.activeControl = styleitem.hitTest(mouseX, mouseY)
        hoverEnabled: true

        property variant control
        property variant pressedX
        property variant pressedY
        property int oldPosition
        property int grooveSize

        Timer {
            running: internal.upPressed || internal.downPressed || internal.pageUpPressed || internal.pageDownPressed
            interval: 350
            onTriggered: internal.autoincrement = true
        }

        Timer {
            running: internal.autoincrement
            interval: 60
            repeat: true
            onTriggered: internal.upPressed ? internal.decrement() : internal.downPressed ? internal.increment() :
                                                                     internal.pageUpPressed ? internal.decrementPage() :
                                                                                              internal.incrementPage()
        }

        onMousePositionChanged: {
            if (pressed && control === "handle") {
                //slider.positionAtMaximum = grooveSize
                if (!styleitem.horizontal)
                    slider.position = oldPosition + (mouseY - pressedY)
                else
                    slider.position = oldPosition + (mouseX - pressedX)
            }
        }

        onPressed: {
            control = styleitem.hitTest(mouseX,mouseY)
            scrollToClickposition = styleitem.styleHint("scrollToClickPosition")
            grooveSize =  styleitem.horizontal? styleitem.subControlRect("groove").width -
                                                styleitem.subControlRect("handle").width:
                                                    styleitem.subControlRect("groove").height -
                                                    styleitem.subControlRect("handle").height;
            if (control == "handle") {
                pressedX = mouseX
                pressedY = mouseY
                oldPosition = slider.position
            } else if (control == "up") {
                decrement();
                upPressed = true
            } else if (control == "down") {
                increment();
                downPressed = true
            } else if (!scrollToClickposition){
                if (control == "upPage") {
                    decrementPage();
                    pageUpPressed = true
                } else if (control == "downPage") {
                    incrementPage();
                    pageDownPressed = true
                }
            } else {
                slider.position = styleitem.horizontal ? mouseX - handleRect.width/2
                                                       : mouseY - handleRect.height/2
            }
        }

        onReleased: {
            autoincrement = false;
            upPressed = false;
            downPressed = false;
            pageUpPressed = false
            pageDownPressed = false
            control = ""
        }

        function incrementPage() {
            value += pageStep
            if (value > maximumValue)
                value = maximumValue
        }

        function decrementPage() {
            value -= pageStep
            if (value < minimumValue)
                value = minimumValue
        }

        function increment() {
            value += singleStep
            if (value > maximumValue)
                value = maximumValue
        }

        function decrement() {
            value -= singleStep
            if (value < minimumValue)
                value = minimumValue
        }

        QStyleItem {
            id: styleitem
            anchors.fill:parent
            elementType: "scrollbar"
            hover: activeControl != "none"
            activeControl: "none"
            sunken: internal.upPressed | internal.downPressed
            minimum: slider.minimumValue
            maximum: slider.maximumValue
            value: slider.value
            horizontal: orientation == Qt.Horizontal
            enabled: parent.enabled
        }

        property variant handleRect: Qt.rect(0,0,0,0)
        property variant grooveRect: Qt.rect(0,0,0,0)
        function updateHandle() {
            internal.handleRect = styleitem.subControlRect("handle")
            grooveRect = styleitem.subControlRect("groove");
        }

        RangeModel {
            id: slider
            minimumValue: 0.0
            maximumValue: 1.0
            value: 0
            stepSize: 0.0
            inverted: false
            positionAtMaximum: internal.grooveSize
        }
    }
}
