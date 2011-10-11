/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

import QtQuick 1.0
import Monitor 1.0

Rectangle {
    id: root

    // ***** properties

    property int candidateHeight: 0
    height: Math.max( candidateHeight, labels.rowCount * 50 + 2 )

    property bool dataAvailable: true
    property int eventCount: 0
    property real progress: 0

    property bool mouseOverSelection : true

    property variant names: [ qsTr("Painting"), qsTr("Compiling"), qsTr("Creating"), qsTr("Binding"), qsTr("Handling Signal")]
    property variant colors : [ "#99CCB3", "#99CCCC", "#99B3CC", "#9999CC", "#CC99B3", "#CC99CC", "#CCCC99", "#CCB399" ]

    property variant mainviewTimePerPixel : 0

    signal updateCursorPosition
    property string fileName: ""
    property int lineNumber: -1

    property int selectedEventIndex : -1
    property bool mouseTracking: false

    property real elapsedTime
    signal updateTimer

    // ***** connections with external objects
    Connections {
        target: zoomControl
        onRangeChanged: {
            var startTime = zoomControl.startTime();
            var endTime = zoomControl.endTime();
            var duration = Math.abs(endTime - startTime);

            mainviewTimePerPixel = duration / root.width;

            backgroundMarks.updateMarks(startTime, endTime);
            view.updateFlickRange(startTime, endTime);
            if (duration > 0) {
                var candidateWidth = qmlEventList.traceEndTime() * flick.width / duration;
                if (flick.contentWidth !== candidateWidth)
                    flick.contentWidth = candidateWidth;
            }

        }
    }

    Connections {
        target: qmlEventList
        onCountChanged: {
            eventCount = qmlEventList.count();
            if (eventCount === 0)
                root.clearAll();
            if (eventCount > 1) {
                root.progress = Math.min(1.0,
                    (qmlEventList.lastTimeMark() - qmlEventList.traceStartTime()) / root.elapsedTime * 1e-9 ) * 0.5;
            } else {
                root.progress = 0;
            }
        }

        onParsingStatusChanged: {
            root.dataAvailable = false;
        }

        onDataReady: {
            if (eventCount > 0) {
                view.clearData();
                view.rebuildCache();
            }
        }
    }

    // ***** functions
    function gotoSourceLocation(file,line) {
        root.fileName = file;
        root.lineNumber = line;
        root.updateCursorPosition();
    }

    function clearData() {
        view.clearData();
        root.dataAvailable = false;
        root.eventCount = 0;
        hideRangeDetails();
    }

    function clearDisplay() {
        clearData();
        selectedEventIndex = -1;
        view.visible = false;
    }

    function clearAll() {
        clearDisplay();
        root.elapsedTime = 0;
        root.updateTimer();
    }

    function nextEvent() {
        if (eventCount > 0) {
            ++selectedEventIndex;
            if (selectedEventIndex >= eventCount)
                selectedEventIndex = 0;
        }
    }

    function prevEvent() {
        if (eventCount > 0) {
            --selectedEventIndex;
            if (selectedEventIndex < 0)
                selectedEventIndex = eventCount - 1;
        }
    }

    function zoomIn() {
        updateZoom( 1/1.1 );
    }

    function zoomOut() {
        updateZoom( 1.1 );
    }

    function updateZoom(factor) {
        var min_length = 1e5; // 0.1 ms
        var windowLength = view.endTime - view.startTime;
        if (windowLength < min_length)
            windowLength = min_length;
        var newWindowLength = windowLength * factor;

        if (newWindowLength > qmlEventList.traceEndTime()) {
            newWindowLength = qmlEventList.traceEndTime();
            factor = newWindowLength / windowLength;
        }
        if (newWindowLength < min_length) {
            newWindowLength = min_length;
            factor = newWindowLength / windowLength;
        }

        var fixedPoint = (view.startTime + view.endTime) / 2;
        if (root.selectedEventIndex !== -1) {
            // center on selected item if it's inside the current screen
            var newFixedPoint = qmlEventList.getStartTime(selectedEventIndex);
            if (newFixedPoint >= view.startTime && newFixedPoint < view.endTime)
                fixedPoint = newFixedPoint;
        }

        var startTime = fixedPoint - factor*(fixedPoint - view.startTime);
        zoomControl.setRange(startTime, startTime + newWindowLength);
    }

    function hideRangeDetails() {
        rangeDetails.visible = false;
        rangeDetails.duration = "";
        rangeDetails.label = "";
        rangeDetails.type = "";
        rangeDetails.file = "";
        rangeDetails.line = -1;

        root.mouseOverSelection = true;
        selectionHighlight.visible = false;
    }

    // ***** slots
    onSelectedEventIndexChanged: {
        if ((!mouseTracking) && eventCount > 0
                && selectedEventIndex > -1 && selectedEventIndex < eventCount) {
            var windowLength = view.endTime - view.startTime;

            var eventStartTime = qmlEventList.getStartTime(selectedEventIndex);
            var eventEndTime = eventStartTime + qmlEventList.getDuration(selectedEventIndex);

            if (eventEndTime < view.startTime || eventStartTime > view.endTime) {
                var center = (eventStartTime + eventEndTime)/2;
                var from = Math.min(qmlEventList.traceEndTime()-windowLength,
                    Math.max(0, Math.floor(center - windowLength/2)));

                zoomControl.setRange(from, from + windowLength);
            }
        }
        if (selectedEventIndex === -1)
            selectionHighlight.visible = false;
    }

    // ***** child items
    Timer {
        id: elapsedTimer
        property date startDate
        property bool reset:  true
        running: connection.recording
        repeat: true
        onRunningChanged: {
            if (running) reset = true;
        }
        interval:  100
        triggeredOnStart: true
        onTriggered: {
            if (reset) {
                startDate = new Date();
                reset = false;
            }
            var time = (new Date() - startDate)/1000;
            root.elapsedTime = time.toFixed(1);
            root.updateTimer();
        }
    }

    TimeMarks {
        id: backgroundMarks
        y: labels.y
        height: flick.height
        anchors.left: flick.left
        anchors.right: flick.right
    }

    Flickable {
        id: flick
        anchors.top: parent.top
        anchors.topMargin: labels.y
        anchors.right: parent.right
        anchors.left: labels.right
        height: root.height
        contentWidth: 0;
        contentHeight: labels.rowCount * 50
        flickableDirection: Flickable.HorizontalFlick

        onContentXChanged: {
            if (Math.round(view.startX) !== contentX)
                view.startX = contentX;
        }

        clip:true

        TimelineView {
            id: view

            eventList: qmlEventList

            width: flick.width
            height: flick.contentHeight

            property variant startX: 0
            onStartXChanged: {
                var newStartTime = Math.round(startX * (endTime - startTime) / flick.width);
                if (Math.abs(newStartTime - startTime) > 1) {
                    var newEndTime = Math.round((startX+flick.width)* (endTime - startTime) / flick.width);
                    zoomControl.setRange(newStartTime, newEndTime);
                }

                if (Math.round(startX) !== flick.contentX)
                    flick.contentX = startX;
            }

            function updateFlickRange(start, end) {
                if (start !== startTime || end !== endTime) {
                    startTime = start;
                    endTime = end;
                    var newStartX = startTime  * flick.width / (endTime-startTime);
                    if (Math.abs(newStartX - startX) >= 1)
                        startX = newStartX;
                    updateTimeline();
                }
            }

            property real timeSpan: endTime - startTime
            onTimeSpanChanged: {
                if (selectedEventIndex !== -1 && selectionHighlight.visible) {
                    var spacing = flick.width / timeSpan;
                    selectionHighlight.x = (qmlEventList.getStartTime(selectedEventIndex) - qmlEventList.traceStartTime()) * spacing;
                    selectionHighlight.width = qmlEventList.getDuration(selectedEventIndex) * spacing;
               }
            }

            onCachedProgressChanged: {
                root.progress = 0.5 + cachedProgress * 0.5;
            }

            onCacheReady: {
                root.progress = 1.0;
                root.dataAvailable = true;
                if (root.eventCount > 0) {
                    view.visible = true;
                    view.updateTimeline();
                    zoomControl.setRange(0, qmlEventList.traceEndTime()/10);
                }
            }

            delegate: Rectangle {
                id: obj
                property color myColor: root.colors[type]

                function conditionalHide() {
                    if (!mouseArea.containsMouse)
                        mouseArea.exited()
                }

                property int baseY: type * view.height / labels.rowCount
                property int baseHeight: view.height / labels.rowCount
                y: baseY + (nestingLevel-1)*(baseHeight / nestingDepth)
                height: baseHeight / nestingDepth
                gradient: Gradient {
                    GradientStop { position: 0.0; color: myColor }
                    GradientStop { position: 0.5; color: Qt.darker(myColor, 1.1) }
                    GradientStop { position: 1.0; color: myColor }
                }
                smooth: true

                property bool componentIsCompleted: false
                Component.onCompleted: {
                    componentIsCompleted = true;
                    updateDetails();
                }

                property bool isSelected: root.selectedEventIndex == index;
                onIsSelectedChanged: {
                    updateDetails();
                }

                function updateDetails() {
                    if (!root.mouseTracking && componentIsCompleted) {
                        if (isSelected) {
                            enableSelected(0, 0);
                        }
                    }
                }

                function enableSelected(x,y) {
                    rangeDetails.duration = qmlEventList.getDuration(index)/1000.0;
                    rangeDetails.label = qmlEventList.getDetails(index);
                    rangeDetails.file = qmlEventList.getFilename(index);
                    rangeDetails.line = qmlEventList.getLine(index);
                    rangeDetails.type = root.names[type];

                    rangeDetails.visible = true;

                    selectionHighlight.x = obj.x;
                    selectionHighlight.y = obj.y;
                    selectionHighlight.width = width;
                    selectionHighlight.height = height;
                    selectionHighlight.visible = true;
                }

                MouseArea {
                    id: mouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    onEntered: {
                        if (root.mouseOverSelection) {
                            root.mouseTracking = true;
                            root.selectedEventIndex = index;
                            enableSelected(mouseX, y);
                            root.mouseTracking = false;
                        }
                    }

                    onPressed: {
                        root.mouseTracking = true;
                        root.selectedEventIndex = index;
                        enableSelected(mouseX, y);
                        root.mouseTracking = false;

                        root.mouseOverSelection = false;
                        root.gotoSourceLocation(rangeDetails.file, rangeDetails.line);
                    }
                }
            }

            Rectangle {
                id: selectionHighlight
                color:"transparent"
                border.width: 2
                border.color: "blue"
                radius: 2
                visible: false
                z:1
            }

            MouseArea {
                width: parent.width
                height: parent.height
                x: flick.contentX
                onClicked: {
                    root.hideRangeDetails();
                }
            }
        }
    }

    RangeDetails {
        id: rangeDetails
    }

    Rectangle {
        id: labels
        width: 150
        color: "#dcdcdc"
        height: flick.contentHeight

        property int rowCount: 5

        Column {
            id: col
            Repeater {
                model: labels.rowCount
                delegate: Label {
                    text: root.names[index]
                    height: labels.height/labels.rowCount
                }
            }
        }

        //right border divider
        Rectangle {
            width: 1
            height: parent.height
            anchors.right: parent.right
            color: "#cccccc"
        }
    }

    Rectangle {
        id: labelsTail
        anchors.top: labels.bottom
        anchors.bottom: root.bottom
        width: labels.width
        color: labels.color
    }

    StatusDisplay {
        anchors.horizontalCenter: flick.horizontalCenter
        anchors.verticalCenter: labels.verticalCenter
        z:3
    }
}
