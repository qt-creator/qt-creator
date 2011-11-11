/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

import QtQuick 1.0

Rectangle {
    id: selectionRange

    width: 1
    color: "transparent"

    property bool ready: visible && creationState === 3

    property color lighterColor:"#6680b2f6"
    property color darkerColor:"#666da1e8"
    property color gapColor: "#336da1e8"
    property color hardBorderColor: "#aa6da1e8"
    property color thinColor: "blue"

    property string startTimeString: detailedPrintTime(startTime)
    property string endTimeString: detailedPrintTime(startTime+duration)
    property string durationString: detailedPrintTime(duration)

    property variant startTime: x * selectionRange.viewTimePerPixel + qmlEventList.traceStartTime()
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
        case 1: color = thinColor; break;
        default: color = lighterColor; break;
        }
    }

    onIsDraggingChanged: {
        if (isDragging)
            color = darkerColor;
        else
            color = lighterColor;
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
        color: darkerColor
        border.color: hardBorderColor
        border.width: 0

        states: State {
            name: "highlighted"
            PropertyChanges {
                target: leftBorder
                width: 3
                border.width: 2
            }
        }

        onXChanged: if (x != 0) {
            selectionRange.width = initialWidth - x;
            selectionRange.x = initialX + x;
            x = 0;
        }

        MouseArea {
            x: -3
            width: 7
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
        color: darkerColor
        border.color: hardBorderColor
        border.width: 0

        states: State {
            name: "highlighted"
            PropertyChanges {
                target: rightBorder
                width: 3
                border.width: 2
            }
        }

        onXChanged: {
            if (x != selectionRange.width) {
                selectionRange.width = x;
            }
        }

        MouseArea {
            x: -3
            width: 7
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
