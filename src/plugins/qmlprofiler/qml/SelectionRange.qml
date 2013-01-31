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

Rectangle {
    id: selectionRange

    width: 1
    color: "transparent"

    property bool ready: visible && creationState === 3

    property color rangeColor:"#444a64b8"
    property color pressedColor:"#664a64b8"
    property color borderColor:"#aa4a64b8"
    property color dragMarkerColor: "#4a64b8"
    property color singleLineColor: "#4a64b8"

    property string startTimeString: detailedPrintTime(startTime)
    property string endTimeString: detailedPrintTime(startTime+duration)
    property string durationString: detailedPrintTime(duration)

    property variant startTime: x * selectionRange.viewTimePerPixel + qmlProfilerDataModel.traceStartTime()
    property variant duration: width * selectionRange.viewTimePerPixel
    property variant viewTimePerPixel: 1
    property variant creationState : 0

    property variant x1
    property variant x2
    property variant x3: Math.min(x1, x2)
    property variant x4: Math.max(x1, x2)

    property bool isDragging: false

    Connections {
        target: zoomControl
        onRangeChanged: {
            var oldTimePerPixel = selectionRange.viewTimePerPixel;
            selectionRange.viewTimePerPixel = Math.abs(zoomControl.endTime() - zoomControl.startTime()) / flick.width;
            if (creationState === 3 && oldTimePerPixel != selectionRange.viewTimePerPixel) {
                selectionRange.x = x * oldTimePerPixel / selectionRange.viewTimePerPixel;
                selectionRange.width = width * oldTimePerPixel / selectionRange.viewTimePerPixel;
            }
        }
    }

    onCreationStateChanged: {
        switch (creationState) {
        case 0: color = "transparent"; break;
        case 1: color = singleLineColor; break;
        default: color = rangeColor; break;
        }
    }

    onIsDraggingChanged: {
        if (isDragging)
            color = pressedColor;
        else
            color = rangeColor;
    }

    function reset(setVisible) {
        width = 1;
        creationState = 0;
        visible = setVisible;
    }

    function setPos(pos) {
        switch (creationState) {
        case 1: {
            width = 1;
            x1 = pos;
            x2 = pos;
            x = pos;
            break;
        }
        case 2: {
            x2 = pos;
            x = x3;
            width = x4-x3;
            break;
        }
        default: return;
        }
    }


    function detailedPrintTime( t )
    {
        if (t <= 0) return "0";
        if (t<1000) return t+" ns";
        t = Math.floor(t/1000);
        if (t<1000) return t+" μs";
        if (t<1e6) return (t/1000) + " ms";
        return (t/1e6) + " s";
    }

    // creation control
    function releasedOnCreation() {
        if (selectionRange.creationState === 2) {
            flick.interactive = true;
            selectionRange.creationState = 3;
            selectionRangeControl.enabled = false;
        }
    }

    function pressedOnCreation() {
        if (selectionRange.creationState === 1) {
            flick.interactive = false;
            selectionRange.setPos(selectionRangeControl.mouseX + flick.contentX);
            selectionRange.creationState = 2;
        }
    }

    function movedOnCreation() {
        if (selectionRange.creationState === 0) {
            selectionRange.creationState = 1;
        }

        if (!root.eventCount)
            return;

        if (!selectionRangeControl.pressed && selectionRange.creationState==3)
            return;

        if (selectionRangeControl.pressed) {
            selectionRange.setPos(selectionRangeControl.mouseX + flick.contentX);
        } else {
            selectionRange.setPos(selectionRangeControl.mouseX + flick.contentX);
        }
    }

    Rectangle {
        id: leftBorder

        visible: selectionRange.creationState === 3

        // used for dragging the borders
        property real initialX: 0
        property real initialWidth: 0

        x: 0
        height: parent.height
        width: 1
        color: borderColor

        Rectangle {
            id: leftBorderHandle
            height: parent.height
            x: -width
            width: 9
            color: "#869cd1"
            visible: false
            Image {
                source: "range_handle.png"
                x: 4
                width: 4
                height: 63
                fillMode: Image.Tile
                y: root.scrollY + root.candidateHeight / 2 - 32
            }
        }

        states: State {
            name: "highlighted"
            PropertyChanges {
                target: leftBorderHandle
                visible: true
            }
        }

        onXChanged: if (x != 0) {
            selectionRange.width = initialWidth - x;
            selectionRange.x = initialX + x;
            x = 0;
        }

        MouseArea {
            x: -12
            width: 15
            y: 0
            height: parent.height

            drag.target: leftBorder
            drag.axis: "XAxis"
            drag.minimumX: -parent.initialX
            drag.maximumX: parent.initialWidth - 2

            hoverEnabled: true

            onEntered: parent.state = "highlighted"
            onExited: {
                if (!pressed) parent.state = "";
            }
            onReleased: {
                if (!containsMouse) parent.state = "" ;
            }
            onPressed: {
                parent.initialX = selectionRange.x;
                parent.initialWidth = selectionRange.width;
            }
        }
    }

    Rectangle {
        id: rightBorder

        visible: selectionRange.creationState === 3

        x: selectionRange.width
        height: parent.height
        width: 1
        color: borderColor

        Rectangle {
            id: rightBorderHandle
            height: parent.height
            x: 1
            width: 9
            color: "#869cd1"
            visible: false
            Image {
                source: "range_handle.png"
                x: 2
                width: 4
                height: 63
                fillMode: Image.Tile
                y: root.scrollY + root.candidateHeight / 2 - 32
            }
        }

        states: State {
            name: "highlighted"
            PropertyChanges {
                target: rightBorderHandle
                visible: true
            }
        }

        onXChanged: {
            if (x != selectionRange.width) {
                selectionRange.width = x;
            }
        }

        MouseArea {
            x: -3
            width: 15
            y: 0
            height: parent.height

            drag.target: rightBorder
            drag.axis: "XAxis"
            drag.minimumX: 1
            drag.maximumX: flick.contentWidth - selectionRange.x

            hoverEnabled: true

            onEntered: {
                parent.state = "highlighted";
            }
            onExited: {
                if (!pressed) parent.state = "";
            }
            onReleased: {
                if (!containsMouse) parent.state = "";
            }
        }
    }
}
