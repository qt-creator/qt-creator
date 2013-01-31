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
    signal selectedEventChanged(int eventId)
    property bool lockItemSelection : false

    property variant names: [ qsTr("Painting"),
                              qsTr("Compiling"),
                              qsTr("Creating"),
                              qsTr("Binding"),
                              qsTr("Handling Signal")]
    property variant colors : [ "#99CCB3", "#99CCCC", "#99B3CC",
        "#9999CC", "#CC99B3", "#CC99CC", "#CCCC99", "#CCB399" ]

    property variant mainviewTimePerPixel : 0

    signal updateCursorPosition
    property string fileName: ""
    property int lineNumber: -1
    property int columnNumber: 0

    signal updateRangeButton
    property bool selectionRangeMode: false

    property bool selectionRangeReady: selectionRange.ready
    property variant selectionRangeStart: selectionRange.startTime
    property variant selectionRangeEnd: selectionRange.startTime + selectionRange.duration

    signal changeToolTip(string text)
    signal updateVerticalScroll(int newPosition)

    property bool recordingEnabled: false
    property bool appKilled : false

    property date recordingStartDate
    property real elapsedTime

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
                var candidateWidth = qmlProfilerDataModel.traceDuration() *
                        flick.width / duration;
                if (flick.contentWidth !== candidateWidth)
                    flick.contentWidth = candidateWidth;
            }

        }
    }

    Connections {
        target: qmlProfilerDataModel
        onCountChanged: {
            eventCount = qmlProfilerDataModel.count();
            if (eventCount === 0)
                root.clearAll();
            if (eventCount > 1) {
                root.progress = Math.min(1.0,
                    (qmlProfilerDataModel.lastTimeMark() -
                     qmlProfilerDataModel.traceStartTime()) / root.elapsedTime * 1e-9 );
            } else {
                root.progress = 0;
            }
        }
        onStateChanged: {
            switch (qmlProfilerDataModel.getCurrentStateFromQml()) {
            case 0: {
                root.clearAll();
                break;
            }
            case 1: {
                root.dataAvailable = false;
                break;
            }
            case 2: {
                root.progress = 0.9; // jump to 90%
                break;
            }
            case 3: {
                view.clearData();
                progress = 1.0;
                dataAvailable = true;
                view.visible = true;
                view.requestPaint();
                zoomControl.setRange(qmlProfilerDataModel.traceStartTime(),
                                     qmlProfilerDataModel.traceStartTime() +
                                     qmlProfilerDataModel.traceDuration()/10);
                break;
            }
            }
        }
    }

    // ***** functions
    function gotoSourceLocation(file,line,column) {
        root.fileName = file;
        root.lineNumber = line;
        root.columnNumber = column;
        root.updateCursorPosition();
    }

    function clearData() {
        view.clearData();
        dataAvailable = false;
        appKilled = false;
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
        elapsedTime = 0;
    }

    function nextEvent() {
        view.selectNext();
    }

    function prevEvent() {
        view.selectPrev();
    }

    function updateWindowLength(absoluteFactor) {
        var windowLength = view.endTime - view.startTime;
        if (qmlProfilerDataModel.traceEndTime() <= qmlProfilerDataModel.traceStartTime() ||
                windowLength <= 0)
            return;
        var currentFactor = windowLength / qmlProfilerDataModel.traceDuration();
        updateZoom(absoluteFactor / currentFactor);
    }

    function updateZoom(relativeFactor) {
        var min_length = 1e5; // 0.1 ms
        var windowLength = view.endTime - view.startTime;
        if (windowLength < min_length)
            windowLength = min_length;
        var newWindowLength = windowLength * relativeFactor;

        if (newWindowLength > qmlProfilerDataModel.traceDuration()) {
            newWindowLength = qmlProfilerDataModel.traceDuration();
            relativeFactor = newWindowLength / windowLength;
        }
        if (newWindowLength < min_length) {
            newWindowLength = min_length;
            relativeFactor = newWindowLength / windowLength;
        }

        var fixedPoint = (view.startTime + view.endTime) / 2;
        if (view.selectedItem !== -1) {
            // center on selected item if it's inside the current screen
            var newFixedPoint = qmlProfilerDataModel.getStartTime(view.selectedItem);
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

        if (newWindowLength > qmlProfilerDataModel.traceDuration()) {
            newWindowLength = qmlProfilerDataModel.traceDuration();
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
        if (newStart + windowLength > qmlProfilerDataModel.traceEndTime())
            newStart = qmlProfilerDataModel.traceEndTime() - windowLength;
        zoomControl.setRange(newStart, newStart + windowLength);
    }

    function recenterOnItem( itemIndex )
    {
        if (itemIndex === -1)
            return;

        // if item is outside of the view, jump back to its position
        if (qmlProfilerDataModel.getEndTime(itemIndex) < view.startTime ||
                qmlProfilerDataModel.getStartTime(itemIndex) > view.endTime) {
            recenter((qmlProfilerDataModel.getStartTime(itemIndex) +
                      qmlProfilerDataModel.getEndTime(itemIndex)) / 2);
        }
    }

    function wheelZoom(wheelCenter, wheelDelta) {
        if (qmlProfilerDataModel.traceEndTime() > qmlProfilerDataModel.traceStartTime() &&
                wheelDelta !== 0) {
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
        rangeDetails.column = 0;
        rangeDetails.isBindingLoop = false;
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
            selectedEventChanged( qmlProfilerDataModel.getEventId(selectedItem) );
            lockItemSelection = false;
        }
    }

    onRecordingEnabledChanged: {
        if (recordingEnabled) {
            recordingStartDate = new Date();
            elapsedTime = 0;
        } else {
            elapsedTime = (new Date() - recordingStartDate)/1000.0;
        }
    }


    // ***** child items
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
                zoomControl.setRange(selectionRange.startTime,
                                     selectionRange.startTime + selectionRange.duration);
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

        TimelineRenderer {
            id: view

            profilerDataModel: qmlProfilerDataModel

            x: flick.contentX
            width: flick.width
            height: root.height

            property variant startX: 0
            onStartXChanged: {
                var newStartTime = Math.round(startX * (endTime - startTime) / flick.width) +
                        qmlProfilerDataModel.traceStartTime();
                if (Math.abs(newStartTime - startTime) > 1) {
                    var newEndTime = Math.round((startX+flick.width) *
                                                (endTime - startTime) /
                                                flick.width) +
                                                qmlProfilerDataModel.traceStartTime();
                    zoomControl.setRange(newStartTime, newEndTime);
                }

                if (Math.round(startX) !== flick.contentX)
                    flick.contentX = startX;
            }

            function updateFlickRange(start, end) {
                if (start !== startTime || end !== endTime) {
                    startTime = start;
                    endTime = end;
                    var newStartX = (startTime - qmlProfilerDataModel.traceStartTime()) *
                            flick.width / (endTime-startTime);
                    if (Math.abs(newStartX - startX) >= 1)
                        startX = newStartX;
                }
            }

            onSelectedItemChanged: {
                if (selectedItem !== -1) {
                    // display details
                    rangeDetails.duration = qmlProfilerDataModel.getDuration(selectedItem)/1000.0;
                    rangeDetails.label = qmlProfilerDataModel.getDetails(selectedItem);
                    rangeDetails.file = qmlProfilerDataModel.getFilename(selectedItem);
                    rangeDetails.line = qmlProfilerDataModel.getLine(selectedItem);
                    rangeDetails.column = qmlProfilerDataModel.getColumn(selectedItem);
                    rangeDetails.type = root.names[qmlProfilerDataModel.getType(selectedItem)];
                    rangeDetails.isBindingLoop = qmlProfilerDataModel.getBindingLoopDest(selectedItem)!==-1;

                    rangeDetails.visible = true;

                    // center view (horizontally)
                    var windowLength = view.endTime - view.startTime;
                    var eventStartTime = qmlProfilerDataModel.getStartTime(selectedItem);
                    var eventEndTime = eventStartTime +
                            qmlProfilerDataModel.getDuration(selectedItem);

                    if (eventEndTime < view.startTime || eventStartTime > view.endTime) {
                        var center = (eventStartTime + eventEndTime)/2;
                        var from = Math.min(qmlProfilerDataModel.traceEndTime()-windowLength,
                                            Math.max(0, Math.floor(center - windowLength/2)));

                        zoomControl.setRange(from, from + windowLength);
                    }

                    // center view (vertically)
                    var itemY = view.getYPosition(selectedItem);
                    if (itemY < root.scrollY) {
                        root.updateVerticalScroll(itemY);
                    } else
                        if (itemY + root.singleRowHeight >
                                root.scrollY + root.candidateHeight) {
                            root.updateVerticalScroll(itemY + root.singleRowHeight -
                                                      root.candidateHeight);
                    }
                } else {
                    root.hideRangeDetails();
                }
            }

            onItemPressed: {
                if (pressedItem !== -1) {
                    root.gotoSourceLocation(qmlProfilerDataModel.getFilename(pressedItem),
                                            qmlProfilerDataModel.getLine(pressedItem),
                                            qmlProfilerDataModel.getColumn(pressedItem));
                }
            }

         // hack to pass mouse events to the other mousearea if enabled
            startDragArea: selectionRangeDrag.enabled ? selectionRangeDrag.x :
                                                        -flick.contentX
            endDragArea: selectionRangeDrag.enabled ?
                             selectionRangeDrag.x + selectionRangeDrag.width :
                             -flick.contentX-1
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
