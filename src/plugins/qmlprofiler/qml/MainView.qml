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

    property alias selectionLocked : view.selectionLocked
    property bool lockItemSelection : false

    property real mainviewTimePerPixel : 0

    signal updateCursorPosition
    property string fileName: ""
    property int lineNumber: -1
    property int columnNumber: 0

    property bool selectionRangeMode: false

    property bool selectionRangeReady: selectionRange.ready
    property real selectionRangeStart: selectionRange.startTime
    property real selectionRangeEnd: selectionRange.startTime + selectionRange.duration

    color: "#dcdcdc"

    // ***** connections with external objects
    Connections {
        target: zoomControl
        onRangeChanged: {
            var startTime = zoomControl.startTime();
            var endTime = zoomControl.endTime();

            mainviewTimePerPixel = Math.abs(endTime - startTime) / root.width;

            backgroundMarks.updateMarks(startTime, endTime);
            view.startTime = startTime;
            view.endTime = endTime;
            view.updateWindow();
        }
        onWindowChanged: {
            view.updateWindow();
        }
    }


    Connections {
        target: qmlProfilerModelProxy
        onDataAvailable: {
            view.clearData();
            zoomControl.setRange(qmlProfilerModelProxy.traceStartTime(),
                                 qmlProfilerModelProxy.traceStartTime() +
                                 qmlProfilerModelProxy.traceDuration()/10);
            view.requestPaint();
        }
        onStateChanged: backgroundMarks.requestPaint()
        onModelsChanged: backgroundMarks.requestPaint()
        onExpandedChanged: backgroundMarks.requestPaint()
        onRowHeightChanged: backgroundMarks.requestPaint()
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
        buttonsBar.updateRangeButton(selectionRangeMode);
        zoomControl.setRange(0,0);
        zoomSlider.externalUpdate = true;
        zoomSlider.value = zoomSlider.minimumValue;
        overview.clear();
        timeDisplay.clear();
    }

    function enableButtonsBar(enable) {
        buttonsBar.enabled = enable;
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

    function recenterOnItem(modelIndex, itemIndex)
    {
        if (itemIndex === -1)
            return;

        // if item is outside of the view, jump back to its position
        if (qmlProfilerModelProxy.endTime(modelIndex, itemIndex) < view.startTime ||
                qmlProfilerModelProxy.startTime(modelIndex, itemIndex) > view.endTime) {
            recenter((qmlProfilerModelProxy.startTime(modelIndex, itemIndex) +
                      qmlProfilerModelProxy.endTime(modelIndex, itemIndex)) / 2);
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

    function selectById(modelIndex, eventId)
    {
        if (eventId === -1 || (modelIndex === view.selectedModel &&
                eventId === qmlProfilerModelProxy.eventId(modelIndex, view.selectedItem)))
            return;

        // this is a slot responding to events from the other pane
        // which tracks only events from the basic model
        if (!lockItemSelection) {
            lockItemSelection = true;
            var itemIndex = view.nextItemFromId(modelIndex, eventId);
            // select an item, lock to it, and recenter if necessary
            view.selectFromId(modelIndex, itemIndex); // triggers recentering
            if (itemIndex !== -1)
                view.selectionLocked = true;
            lockItemSelection = false;
        }
    }

    // ***** slots
    onSelectionRangeModeChanged: {
        selectionRangeControl.enabled = selectionRangeMode;
        selectionRange.reset();
        buttonsBar.updateRangeButton(selectionRangeMode);
    }

    onSelectionLockedChanged: {
        buttonsBar.updateLockButton(selectionLocked);
    }

    focus: true
    property bool shiftPressed: false;
    Keys.onPressed: shiftPressed = (event.key === Qt.Key_Shift);
    Keys.onReleased: shiftPressed = false;

    Flickable {
        id: labelsflick
        flickableDirection: Flickable.VerticalFlick
        interactive: false
        anchors.top: buttonsBar.bottom
        anchors.bottom: overview.top
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

            property int rowCount: qmlProfilerModelProxy.modelCount();

            Column {
                id: col

                // Dispatch the cursor shape to all labels. When dragging the DropArea receiving
                // the drag events is not necessarily related to the MouseArea receiving the mouse
                // events, so we can't use the drag events to determine the cursor shape.
                property bool dragging: false

                Repeater {
                    model: labels.rowCount
                    delegate: CategoryLabel {
                        dragging: col.dragging
                        reverseSelect: root.shiftPressed
                        onDragStarted: col.dragging = true
                        onDragStopped: col.dragging = false
                        draggerParent: labels
                    }
                }
            }
        }
    }

    // border between labels and timeline
    Rectangle {
        id: labelsborder
        anchors.left: labelsflick.right
        anchors.top: parent.top
        anchors.bottom: overview.top
        width: 1
        color: "#858585"
    }

    ButtonsBar {
        id: buttonsBar
        enabled: false
        anchors.top: parent.top
        anchors.left: parent.left
        width: 150
        height: 24
        onZoomControlChanged: zoomSliderToolBar.visible = !zoomSliderToolBar.visible
        onFilterMenuChanged: filterMenu.visible = !filterMenu.visible
        onJumpToNext: view.selectNext();
        onJumpToPrev: view.selectPrev();
        onRangeSelectChanged: selectionRangeMode = rangeButtonChecked();
        onLockChanged: selectionLocked = !lockButtonChecked();
    }

    TimeDisplay {
        id: timeDisplay
        anchors.top: parent.top
        anchors.left: labelsborder.right
        anchors.right: parent.right
        height: 24
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

        onWidthChanged: {
            var duration = Math.abs(zoomControl.endTime() - zoomControl.startTime());
            if (duration > 0)
                contentWidth = zoomControl.windowLength() * width / duration;
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

            property int intX: x
            property int intWidth: width
            onIntXChanged: {
                // Don't updateZoomControl if we're just updating the flick range, _from_
                // zoomControl. The other way round is OK. We _want_ the flick range to be updated
                // on external changes to zoomControl.
                if (recursionGuard)
                    return;

                var newStartTime = intX * (endTime - startTime) / intWidth +
                        zoomControl.windowStart();
                if (Math.abs(newStartTime - startTime) >= 1) {
                    var newEndTime = (intX + intWidth) * (endTime - startTime) / intWidth +
                            zoomControl.windowStart();
                    zoomControl.setRange(newStartTime, newEndTime);
                }
            }

            function updateWindow() {
                var duration = zoomControl.duration();
                if (recursionGuard || duration <= 0)
                    return;

                recursionGuard = true;

                if (!flick.movingHorizontally) {
                    // This triggers an unwanted automatic change in contentX. We ignore that by
                    // checking recursionGuard in this function and in updateZoomControl.
                    flick.contentWidth = zoomControl.windowLength() * intWidth / duration;

                    var newStartX = (startTime - zoomControl.windowStart()) * intWidth /
                            duration;

                    if (isFinite(newStartX) && Math.abs(newStartX - intX) >= 1)
                        flick.contentX = newStartX;
                }
                recursionGuard = false;
            }

            onSelectionChanged: {
                if (selectedItem !== -1) {
                    // display details
                    rangeDetails.showInfo(qmlProfilerModelProxy.details(selectedModel,
                                                                        selectedItem));
                    rangeDetails.setLocation(qmlProfilerModelProxy.location(selectedModel,
                                                                            selectedItem));

                    // center view (horizontally)
                    recenterOnItem(selectedModel, selectedItem);
                    if (!lockItemSelection) {
                        lockItemSelection = true;
                        // update in other views
                        var eventLocation = qmlProfilerModelProxy.location(view.selectedModel,
                                                                           view.selectedItem);
                        gotoSourceLocation(eventLocation.file, eventLocation.line,
                                           eventLocation.column);
                        lockItemSelection = false;
                    }
                } else {
                    root.hideRangeDetails();
                }
            }

            onItemPressed: {
                var location = qmlProfilerModelProxy.location(modelIndex, pressedItem);
                if (location.hasOwnProperty("file")) // not empty
                    root.gotoSourceLocation(location.file, location.line, location.column);
            }

         // hack to pass mouse events to the other mousearea if enabled
            startDragArea: selectionRange.ready ? selectionRange.rangeLeft : -x
            endDragArea: selectionRange.ready ? selectionRange.rangeRight : -x - 1
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
        anchors.top: timeDisplay.bottom
        anchors.bottom: overview.top
        anchors.right: parent.right
    }

    SelectionRangeDetails {
        id: selectionRangeDetails
        visible: selectionRange.visible
        startTime: selectionRange.startTimeString
        duration: selectionRange.durationString
        endTime: selectionRange.endTimeString
        showDuration: selectionRange.rangeWidth > 1
    }

    RangeDetails {
        id: rangeDetails
    }

    Rectangle {
        id: filterMenu
        color: "#9b9b9b"
        enabled: buttonsBar.enabled
        visible: false
        width: labels.width
        anchors.left: parent.left
        anchors.top: buttonsBar.bottom
        height: qmlProfilerModelProxy.modelCount() * buttonsBar.height

        Repeater {
            id: filterMenuInner
            model: qmlProfilerModelProxy.models
            CheckBox {
                anchors.left: filterMenu.left
                anchors.right: filterMenu.right
                height: buttonsBar.height
                y: index * height
                text: qmlProfilerModelProxy.models[index].displayName
                enabled: !qmlProfilerModelProxy.models[index].empty
                checked: enabled && !qmlProfilerModelProxy.models[index].hidden
                onCheckedChanged: qmlProfilerModelProxy.models[index].hidden = !checked
            }
        }

    }

    Rectangle {
        id: zoomSliderToolBar
        objectName: "zoomSliderToolBar"
        color: "#9b9b9b"
        enabled: buttonsBar.enabled
        visible: false
        width: labels.width
        height: buttonsBar.height
        anchors.left: parent.left
        anchors.top: buttonsBar.bottom

        function updateZoomLevel() {
            zoomSlider.externalUpdate = true;
            zoomSlider.value = Math.pow((view.endTime - view.startTime) /
                                        zoomControl.windowLength(),
                                        1 / zoomSlider.exponent) * zoomSlider.maximumValue;
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
                if (externalUpdate || zoomControl.windowEnd() <= zoomControl.windowStart()) {
                    // Zoom range is independently updated. We shouldn't mess
                    // with it here as otherwise we might introduce rounding
                    // or arithmetic errors.
                    externalUpdate = false;
                    return;
                }

                var windowLength = Math.max(
                            Math.pow(value / maximumValue, exponent) * zoomControl.windowLength(),
                            minWindowLength);

                var fixedPoint = (view.startTime + view.endTime) / 2;
                if (view.selectedItem !== -1) {
                    // center on selected item if it's inside the current screen
                    var newFixedPoint = qmlProfilerModelProxy.startTime(view.selectedModel, view.selectedItem);
                    if (newFixedPoint >= view.startTime && newFixedPoint < view.endTime)
                        fixedPoint = newFixedPoint;
                }

                var startTime = Math.max(zoomControl.windowStart(), fixedPoint - windowLength / 2)
                zoomControl.setRange(startTime, startTime + windowLength);
            }
        }
    }

    Overview {
        id: overview
        height: 50
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.left: parent.left
    }
}
