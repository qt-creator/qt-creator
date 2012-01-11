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
import Monitor 1.0

Rectangle {
    id: root

    // ***** properties

    property int candidateHeight: 0
    property int scrollY: 0
    height: Math.max( candidateHeight, labels.height + 2 )

    property int singleRowHeight: 30

    property bool dataAvailable: true
    property int eventCount: 0
    property real progress: 0

    property alias selectionLocked : view.selectionLocked
    signal updateLockButton
    property alias selectedItem: view.selectedItem
    signal selectedEventIdChanged(int eventId)
    property bool lockItemSelection : false

    property variant names: [ qsTr("Painting"), qsTr("Compiling"), qsTr("Creating"), qsTr("Binding"), qsTr("Handling Signal")]
    property variant colors : [ "#99CCB3", "#99CCCC", "#99B3CC", "#9999CC", "#CC99B3", "#CC99CC", "#CCCC99", "#CCB399" ]

    property variant mainviewTimePerPixel : 0

    signal updateCursorPosition
    property string fileName: ""
    property int lineNumber: -1

    property real elapsedTime
    signal updateTimer

    signal updateRangeButton
    property bool selectionRangeMode: false

    property bool selectionRangeReady: selectionRange.ready
    property variant selectionRangeStart: selectionRange.startTime
    property variant selectionRangeEnd: selectionRange.startTime + selectionRange.duration

    signal changeToolTip(string text)

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
                var candidateWidth = qmlEventList.traceDuration() * flick.width / duration;
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
                    (qmlEventList.lastTimeMark() - qmlEventList.traceStartTime()) / root.elapsedTime * 1e-9 );
            } else {
                root.progress = 0;
            }
        }

        onProcessingData: {
            root.dataAvailable = false;
        }

        onPostProcessing: {
            root.progress = 0.9; // jump to 90%
        }

        onDataReady: {
            if (eventCount > 0) {
                view.clearData();
                progress = 1.0;
                dataAvailable = true;
                view.visible = true;
                view.requestPaint();
                zoomControl.setRange(qmlEventList.traceStartTime(), qmlEventList.traceStartTime() + qmlEventList.traceDuration()/10);
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
        dataAvailable = false;
        eventCount = 0;
        hideRangeDetails();
        selectionRangeMode = false;
        updateRangeButton();
        zoomControl.setRange(0,0);
    }

    function clearDisplay() {
        clearData();
        view.visible = false;
    }

    function clearAll() {
        clearDisplay();
        root.elapsedTime = 0;
        root.updateTimer();
    }

    function nextEvent() {
        view.selectNext();
    }

    function prevEvent() {
        view.selectPrev();
    }

    function updateWindowLength(absoluteFactor) {
        var windowLength = view.endTime - view.startTime;
        if (qmlEventList.traceEndTime() <= qmlEventList.traceStartTime() || windowLength <= 0)
            return;
        var currentFactor = windowLength / qmlEventList.traceDuration();
        updateZoom(absoluteFactor / currentFactor);
    }

    function updateZoom(relativeFactor) {
        var min_length = 1e5; // 0.1 ms
        var windowLength = view.endTime - view.startTime;
        if (windowLength < min_length)
            windowLength = min_length;
        var newWindowLength = windowLength * relativeFactor;

        if (newWindowLength > qmlEventList.traceDuration()) {
            newWindowLength = qmlEventList.traceDuration();
            relativeFactor = newWindowLength / windowLength;
        }
        if (newWindowLength < min_length) {
            newWindowLength = min_length;
            relativeFactor = newWindowLength / windowLength;
        }

        var fixedPoint = (view.startTime + view.endTime) / 2;
        if (view.selectedItem !== -1) {
            // center on selected item if it's inside the current screen
            var newFixedPoint = qmlEventList.getStartTime(view.selectedItem);
            if (newFixedPoint >= view.startTime && newFixedPoint < view.endTime)
                fixedPoint = newFixedPoint;
        }

        var startTime = fixedPoint - relativeFactor*(fixedPoint - view.startTime);
        zoomControl.setRange(startTime, startTime + newWindowLength);
    }

    function updateZoomCentered(centerX, relativeFactor)
    {
        var min_length = 1e5; // 0.1 ms
        var windowLength = view.endTime - view.startTime;
        if (windowLength < min_length)
            windowLength = min_length;
        var newWindowLength = windowLength * relativeFactor;

        if (newWindowLength > qmlEventList.traceDuration()) {
            newWindowLength = qmlEventList.traceDuration();
            relativeFactor = newWindowLength / windowLength;
        }
        if (newWindowLength < min_length) {
            newWindowLength = min_length;
            relativeFactor = newWindowLength / windowLength;
        }

        var fixedPoint = (centerX - flick.x) * windowLength / flick.width + view.startTime;
        var startTime = fixedPoint - relativeFactor*(fixedPoint - view.startTime);
        zoomControl.setRange(startTime, startTime + newWindowLength);
    }

    function recenter( centerPoint ) {
        var windowLength = view.endTime - view.startTime;
        var newStart = Math.floor(centerPoint - windowLength/2);
        if (newStart < 0)
            newStart = 0;
        if (newStart + windowLength > qmlEventList.traceEndTime())
            newStart = qmlEventList.traceEndTime() - windowLength;
        zoomControl.setRange(newStart, newStart + windowLength);
    }

    function recenterOnItem( itemIndex )
    {
        if (itemIndex === -1)
            return;

        // if item is outside of the view, jump back to its position
        if (qmlEventList.getEndTime(itemIndex) < view.startTime || qmlEventList.getStartTime(itemIndex) > view.endTime) {
            recenter((qmlEventList.getStartTime(itemIndex) + qmlEventList.getEndTime(itemIndex)) / 2);
        }
    }

    function globalZoom() {
        zoomControl.setRange(qmlEventList.traceStartTime(), qmlEventList.traceEndTime());
    }

    function wheelZoom(wheelCenter, wheelDelta) {
        if (qmlEventList.traceEndTime() > qmlEventList.traceStartTime() && wheelDelta !== 0) {
            if (wheelDelta>0)
                updateZoomCentered(wheelCenter, 1/1.2);
            else
                updateZoomCentered(wheelCenter, 1.2);
        }
    }

    function hideRangeDetails() {
        rangeDetails.visible = false;
        rangeDetails.duration = "";
        rangeDetails.label = "";
        rangeDetails.type = "";
        rangeDetails.file = "";
        rangeDetails.line = -1;
    }

    function selectNextWithId( eventId )
    {
        if (!lockItemSelection) {
            lockItemSelection = true;
            var itemIndex = view.nextItemFromId( eventId );
            // select an item, lock to it, and recenter if necessary
            if (view.selectedItem != itemIndex) {
                view.selectedItem = itemIndex;
                if (itemIndex !== -1) {
                    view.selectionLocked = true;
                    recenterOnItem(itemIndex);
                }
            }
            lockItemSelection = false;
        }
    }

    // ***** slots
    onSelectionRangeModeChanged: {
        selectionRangeControl.enabled = selectionRangeMode;
        selectionRange.reset(selectionRangeMode);
    }

    onSelectionLockedChanged: {
        updateLockButton();
    }

    onSelectedItemChanged: {
        if (selectedItem != -1 && !lockItemSelection) {
            lockItemSelection = true;
            selectedEventIdChanged( qmlEventList.getEventId(selectedItem) );
            lockItemSelection = false;
        }
    }

    // ***** child items
    Timer {
        id: elapsedTimer
        property date startDate
        property bool reset: true
        running: connection.recording && connection.enabled
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
        contentHeight: labels.height
        flickableDirection: Flickable.HorizontalFlick

        onContentXChanged: {
            if (Math.round(view.startX) !== contentX)
                view.startX = contentX;
        }

        clip:true

        MouseArea {
            id: selectionRangeDrag
            enabled: selectionRange.ready
            anchors.fill: selectionRange
            drag.target: selectionRange
            drag.axis: "XAxis"
            drag.minimumX: 0
            drag.maximumX: flick.contentWidth - selectionRange.width
            onPressed: {
                selectionRange.isDragging = true;
            }
            onReleased: {
                selectionRange.isDragging = false;
            }
            onDoubleClicked: {
                zoomControl.setRange(selectionRange.startTime, selectionRange.startTime + selectionRange.duration);
                root.selectionRangeMode = false;
                root.updateRangeButton();
            }
        }


        SelectionRange {
            id: selectionRange
            visible: root.selectionRangeMode
            height: root.height
            z: 2
        }

        TimelineView {
            id: view

            eventList: qmlEventList

            x: flick.contentX
            width: flick.width
            height: root.height

            property variant startX: 0
            onStartXChanged: {
                var newStartTime = Math.round(startX * (endTime - startTime) / flick.width) + qmlEventList.traceStartTime();
                if (Math.abs(newStartTime - startTime) > 1) {
                    var newEndTime = Math.round((startX+flick.width)* (endTime - startTime) / flick.width) + qmlEventList.traceStartTime();
                    zoomControl.setRange(newStartTime, newEndTime);
                }

                if (Math.round(startX) !== flick.contentX)
                    flick.contentX = startX;
            }

            function updateFlickRange(start, end) {
                if (start !== startTime || end !== endTime) {
                    startTime = start;
                    endTime = end;
                    var newStartX = (startTime - qmlEventList.traceStartTime())  * flick.width / (endTime-startTime);
                    if (Math.abs(newStartX - startX) >= 1)
                        startX = newStartX;
                }
            }

            onSelectedItemChanged: {
                if (selectedItem !== -1) {
                    // display details
                    rangeDetails.duration = qmlEventList.getDuration(selectedItem)/1000.0;
                    rangeDetails.label = qmlEventList.getDetails(selectedItem);
                    rangeDetails.file = qmlEventList.getFilename(selectedItem);
                    rangeDetails.line = qmlEventList.getLine(selectedItem);
                    rangeDetails.type = root.names[qmlEventList.getType(selectedItem)];

                    rangeDetails.visible = true;

                    // center view
                    var windowLength = view.endTime - view.startTime;
                    var eventStartTime = qmlEventList.getStartTime(selectedItem);
                    var eventEndTime = eventStartTime + qmlEventList.getDuration(selectedItem);

                    if (eventEndTime < view.startTime || eventStartTime > view.endTime) {
                        var center = (eventStartTime + eventEndTime)/2;
                        var from = Math.min(qmlEventList.traceEndTime()-windowLength,
                                            Math.max(0, Math.floor(center - windowLength/2)));

                        zoomControl.setRange(from, from + windowLength);
                    }
                } else {
                    root.hideRangeDetails();
                }
            }

            onItemPressed: {
                if (pressedItem !== -1) {
                    root.gotoSourceLocation(qmlEventList.getFilename(pressedItem), qmlEventList.getLine(pressedItem));
                }
            }

         // hack to pass mouse events to the other mousearea if enabled
            startDragArea: selectionRangeDrag.enabled ? selectionRangeDrag.x : -flick.contentX
            endDragArea: selectionRangeDrag.enabled ?
                             selectionRangeDrag.x + selectionRangeDrag.width : -flick.contentX-1
        }
        MouseArea {
            id: selectionRangeControl
            enabled: false
            width: flick.width
            height: root.height
            x: flick.contentX
            hoverEnabled: enabled
            z: 2

            onReleased:  {
                selectionRange.releasedOnCreation();
            }
            onPressed:  {
                selectionRange.pressedOnCreation();
            }
            onMousePositionChanged: {
                selectionRange.movedOnCreation();
            }
        }
    }

    SelectionRangeDetails {
        id: selectionRangeDetails
        visible: root.selectionRangeMode
        startTime: selectionRange.startTimeString
        duration: selectionRange.durationString
        endTime: selectionRange.endTimeString
        showDuration: selectionRange.width > 1
    }

    RangeDetails {
        id: rangeDetails
    }

    Rectangle {
        id: labels
        width: 150
        color: "#dcdcdc"
        height: col.height

        property int rowCount: 5
        property variant rowExpanded: [false,false,false,false,false];

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

    // Gradient borders
    Item {
        anchors.left: labels.right
        width: 6
        anchors.top: root.top
        anchors.bottom: root.bottom
        Rectangle {
            x: parent.width
            transformOrigin: Item.TopLeft
            rotation: 90
            width: parent.height
            height: parent.width
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#00000000"; }
                GradientStop { position: 1.0; color: "#86000000"; }
            }
        }
    }

    Item {
        anchors.right: root.right
        width: 6
        anchors.top: root.top
        anchors.bottom: root.bottom
        Rectangle {
            x: parent.width
            transformOrigin: Item.TopLeft
            rotation: 90
            width: parent.height
            height: parent.width
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#86000000"; }
                GradientStop { position: 1.0; color: "#00000000"; }
            }
        }
    }

    Rectangle {
        y: root.scrollY + root.candidateHeight - height
        height: 6
        width: root.width
        x: 0
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#00000000"; }
            GradientStop { position: 1.0; color: "#86000000"; }
        }
    }
}
