/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

import QtQuick 2.1
import Monitor 1.0
import QtQuick.Controls 1.0

Rectangle {
    id: root

    // ***** properties

    property int singleRowHeight: 30

    property alias selectionLocked : view.selectionLocked
    signal updateLockButton
    property alias selectedItem: view.selectedItem
    signal selectedEventChanged(int eventId)
    property bool lockItemSelection : false

    property real mainviewTimePerPixel : 0

    signal updateCursorPosition
    property string fileName: ""
    property int lineNumber: -1
    property int columnNumber: 0

    signal updateRangeButton
    property bool selectionRangeMode: false

    property bool selectionRangeReady: selectionRange.ready
    property real selectionRangeStart: selectionRange.startTime
    property real selectionRangeEnd: selectionRange.startTime + selectionRange.duration

    signal changeToolTip(string text)

    color: "#dcdcdc"

    // ***** connections with external objects
    Connections {
        target: zoomControl
        onRangeChanged: {
            var startTime = zoomControl.startTime();
            var endTime = zoomControl.endTime();

            mainviewTimePerPixel = Math.abs(endTime - startTime) / root.width;

            backgroundMarks.updateMarks(startTime, endTime);
            view.updateFlickRange(startTime, endTime);
        }
    }


    Connections {
        target: qmlProfilerModelProxy
        onStateChanged: {
            // Clear if model is empty.
            if (qmlProfilerModelProxy.getState() === 0)
                root.clear();
        }
        onDataAvailable: {
            view.clearData();
            zoomControl.setRange(qmlProfilerModelProxy.traceStartTime(),
                                 qmlProfilerModelProxy.traceStartTime() +
                                 qmlProfilerModelProxy.traceDuration()/10);
            view.requestPaint();
        }
    }


    // ***** functions
    function gotoSourceLocation(file,line,column) {
        if (file !== undefined) {
            root.fileName = file;
            root.lineNumber = line;
            root.columnNumber = column;
            root.updateCursorPosition();
        }
    }

    function clear() {
        flick.contentY = 0;
        flick.contentX = 0;
        flick.contentWidth = 0;
        view.clearData();
        view.startTime = view.endTime = 0;
        hideRangeDetails();
        selectionRangeMode = false;
        updateRangeButton();
        zoomControl.setRange(0,0);
        zoomSlider.externalUpdate = true;
        zoomSlider.value = zoomSlider.minimumValue;
    }

    function nextEvent() {
        view.selectNext();
    }

    function prevEvent() {
        view.selectPrev();
    }

    function recenter( centerPoint ) {
        var windowLength = view.endTime - view.startTime;
        var newStart = Math.floor(centerPoint - windowLength/2);
        if (newStart < 0)
            newStart = 0;
        if (newStart + windowLength > qmlProfilerModelProxy.traceEndTime())
            newStart = qmlProfilerModelProxy.traceEndTime() - windowLength;
        zoomControl.setRange(newStart, newStart + windowLength);
    }

    function recenterOnItem( modelIndex, itemIndex )
    {
        if (itemIndex === -1)
            return;

        // if item is outside of the view, jump back to its position
        if (qmlProfilerModelProxy.getEndTime(modelIndex, itemIndex) < view.startTime ||
                qmlProfilerModelProxy.getStartTime(modelIndex, itemIndex) > view.endTime) {
            recenter((qmlProfilerModelProxy.getStartTime(modelIndex, itemIndex) +
                      qmlProfilerModelProxy.getEndTime(modelIndex, itemIndex)) / 2);
        }

    }

    function hideRangeDetails() {
        rangeDetails.visible = false;
        rangeDetails.duration = "";
        rangeDetails.label = "";
        rangeDetails.file = "";
        rangeDetails.line = -1;
        rangeDetails.column = 0;
        rangeDetails.isBindingLoop = false;
    }

    function selectNextByHash(hash) {
        var eventId = qmlProfilerModelProxy.getEventIdForHash(hash);
        if (eventId !== -1) {
            selectNextById(eventId);
        }
    }

    function selectNextById(eventId)
    {
        // this is a slot responding to events from the other pane
        // which tracks only events from the basic model
        if (!lockItemSelection) {
            lockItemSelection = true;
            var modelIndex = qmlProfilerModelProxy.basicModelIndex();
            var itemIndex = view.nextItemFromId( modelIndex, eventId );
            // select an item, lock to it, and recenter if necessary
            if (view.selectedItem != itemIndex || view.selectedModel != modelIndex) {
                view.selectedModel = modelIndex;
                view.selectedItem = itemIndex;
                if (itemIndex !== -1) {
                    view.selectionLocked = true;
                    recenterOnItem(modelIndex, itemIndex);
                }
            }
            lockItemSelection = false;
        }
    }

    // ***** slots
    onSelectionRangeModeChanged: {
        selectionRangeControl.enabled = selectionRangeMode;
        selectionRange.reset();
    }

    onSelectionLockedChanged: {
        updateLockButton();
    }

    onSelectedItemChanged: {
        if (selectedItem != -1 && !lockItemSelection) {
            lockItemSelection = true;
            // update in other views
            var eventLocation = qmlProfilerModelProxy.getEventLocation(view.selectedModel, view.selectedItem);
            gotoSourceLocation(eventLocation.file, eventLocation.line, eventLocation.column);
            lockItemSelection = false;
        }
    }

    Flickable {
        id: labelsflick
        flickableDirection: Flickable.VerticalFlick
        interactive: false
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        width: labels.width
        contentY: flick.contentY

        // reserve some more space than needed to prevent weird effects when resizing
        contentHeight: labels.height + height

        Rectangle {
            id: labels
            anchors.left: parent.left
            width: 150
            color: root.color
            height: col.height

            property int rowCount: qmlProfilerModelProxy.categoryCount();

            Column {
                id: col
                Repeater {
                    model: labels.rowCount
                    delegate: CategoryLabel { }
                }
            }
        }
    }

    // border between labels and timeline
    Rectangle {
        id: labelsborder
        anchors.left: labelsflick.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: 1
        color: "#858585"
    }

    Flickable {
        id: flick
        contentHeight: labels.height
        contentWidth: 0
        flickableDirection: Flickable.HorizontalAndVerticalFlick
        boundsBehavior: Flickable.StopAtBounds
        clip:true

        // ScrollView will try to deinteractivate it. We don't want that
        // as the horizontal flickable is interactive, too. We do occasionally
        // switch to non-interactive ourselves, though.
        property bool stayInteractive: true
        onInteractiveChanged: interactive = stayInteractive
        onStayInteractiveChanged: interactive = stayInteractive

        onContentXChanged: view.updateZoomControl()
        onWidthChanged: {
            var duration = Math.abs(zoomControl.endTime() - zoomControl.startTime());
            if (duration > 0)
                contentWidth = qmlProfilerModelProxy.traceDuration() * width / duration;
        }

        // ***** child items
        TimeMarks {
            id: backgroundMarks
            y: flick.contentY
            x: flick.contentX
            height: flick.height
            width: scroller.width
        }

        SelectionRange {
            id: selectionRange
            visible: root.selectionRangeMode && creationState !== 0
            z: 2
        }

        TimelineRenderer {
            id: view

            profilerModelProxy: qmlProfilerModelProxy

            x: flick.contentX
            y: flick.contentY

            // paint "under" the vertical scrollbar, so that it always matches with the timemarks
            width: scroller.width
            height: flick.height

            onEndTimeChanged: requestPaint()
            onYChanged: requestPaint()
            onHeightChanged: requestPaint()
            property bool recursionGuard: false

            function updateZoomControl() {
                // Don't updateZoomControl if we're just updating the flick range, _from_
                // zoomControl. The other way round is OK. We _want_ the flick range to be updated
                // on external changes to zoomControl.
                if (recursionGuard)
                    return;

                var newStartTime = Math.round(flick.contentX * (endTime - startTime) / flick.width) +
                        qmlProfilerModelProxy.traceStartTime();
                if (Math.abs(newStartTime - startTime) > 1) {
                    var newEndTime = Math.round((flick.contentX + flick.width) *
                                                (endTime - startTime) /
                                                flick.width) +
                                                qmlProfilerModelProxy.traceStartTime();
                    zoomControl.setRange(newStartTime, newEndTime);
                }
            }

            function updateFlickRange(start, end) {
                var duration = end - start;
                if (recursionGuard || duration <= 0 || (start === startTime && end === endTime))
                    return;

                recursionGuard = true;

                startTime = start;
                endTime = end;
                if (!flick.flickingHorizontally) {
                    // This triggers an unwanted automatic change in contentX. We ignore that by
                    // checking recursionGuard in this function and in updateZoomControl.
                    flick.contentWidth = qmlProfilerModelProxy.traceDuration() * flick.width /
                            duration;

                    var newStartX = (startTime - qmlProfilerModelProxy.traceStartTime()) *
                            flick.width / duration;

                    if (isFinite(newStartX) && Math.abs(newStartX - flick.contentX) >= 1)
                        flick.contentX = newStartX;
                }
                recursionGuard = false;
            }

            onSelectedItemChanged: {
                if (selectedItem !== -1) {
                    // display details
                    rangeDetails.showInfo(qmlProfilerModelProxy.getEventDetails(selectedModel, selectedItem));
                    rangeDetails.setLocation(qmlProfilerModelProxy.getEventLocation(selectedModel, selectedItem));

                    // center view (horizontally)
                    var windowLength = view.endTime - view.startTime;
                    var eventStartTime = qmlProfilerModelProxy.getStartTime(selectedModel, selectedItem);
                    var eventEndTime = eventStartTime +
                            qmlProfilerModelProxy.getDuration(selectedModel, selectedItem);

                    if (eventEndTime < view.startTime || eventStartTime > view.endTime) {
                        var center = (eventStartTime + eventEndTime)/2;
                        var from = Math.min(qmlProfilerModelProxy.traceEndTime()-windowLength,
                                            Math.max(0, Math.floor(center - windowLength/2)));

                        zoomControl.setRange(from, from + windowLength);

                    }
                } else {
                    root.hideRangeDetails();
                }
            }

            onItemPressed: {
                var location = qmlProfilerModelProxy.getEventLocation(modelIndex, pressedItem);
                if (location.hasOwnProperty("file")) // not empty
                    root.gotoSourceLocation(location.file, location.line, location.column);
            }

         // hack to pass mouse events to the other mousearea if enabled
            startDragArea: selectionRange.ready ? selectionRange.getLeft() : -flick.contentX
            endDragArea: selectionRange.ready ? selectionRange.getRight() : -flick.contentX-1
        }
        MouseArea {
            id: selectionRangeControl
            enabled: false
            width: flick.width
            height: flick.height
            x: flick.contentX
            y: flick.contentY
            hoverEnabled: enabled
            z: 2

            onReleased:  {
                selectionRange.releasedOnCreation();
            }
            onPressed:  {
                selectionRange.pressedOnCreation();
            }
            onCanceled: {
                selectionRange.releasedOnCreation();
            }
            onPositionChanged: {
                selectionRange.movedOnCreation();
            }
        }
    }

    ScrollView {
        id: scroller
        contentItem: flick
        anchors.left: labelsborder.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.right: parent.right
    }

    SelectionRangeDetails {
        id: selectionRangeDetails
        visible: selectionRange.visible
        startTime: selectionRange.startTimeString
        duration: selectionRange.durationString
        endTime: selectionRange.endTimeString
        showDuration: selectionRange.getWidth() > 1
    }

    RangeDetails {
        id: rangeDetails
    }

    Rectangle {
        objectName: "zoomSliderToolBar"
        color: "#9b9b9b"
        enabled: false
        visible: false
        width: labels.width
        height: 24
        x: 0
        y: 0

        function updateZoomLevel() {
            zoomSlider.externalUpdate = true;
            zoomSlider.value = Math.pow((view.endTime - view.startTime) / qmlProfilerModelProxy.traceDuration(), 1 / zoomSlider.exponent) * zoomSlider.maximumValue;
        }


        Slider {
            id: zoomSlider
            anchors.fill: parent
            minimumValue: 1
            maximumValue: 10000
            stepSize: 100

            property int exponent: 3
            property bool externalUpdate: false
            property int minWindowLength: 1e5 // 0.1 ms

            onValueChanged: {
                if (externalUpdate || qmlProfilerModelProxy.traceEndTime() <= qmlProfilerModelProxy.traceStartTime()) {
                    // Zoom range is independently updated. We shouldn't mess
                    // with it here as otherwise we might introduce rounding
                    // or arithmetic errors.
                    externalUpdate = false;
                    return;
                }

                var windowLength = Math.max(
                            Math.pow(value / maximumValue, exponent) * qmlProfilerModelProxy.traceDuration(),
                            minWindowLength);

                var fixedPoint = (view.startTime + view.endTime) / 2;
                if (view.selectedItem !== -1) {
                    // center on selected item if it's inside the current screen
                    var newFixedPoint = qmlProfilerModelProxy.getStartTime(view.selectedModel, view.selectedItem);
                    if (newFixedPoint >= view.startTime && newFixedPoint < view.endTime)
                        fixedPoint = newFixedPoint;
                }

                var startTime = Math.max(qmlProfilerModelProxy.traceStartTime(), fixedPoint - windowLength / 2)
                zoomControl.setRange(startTime, startTime + windowLength);
            }
        }
    }
}
