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
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
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

    property real mainviewTimePerPixel: zoomControl.rangeDuration / root.width

    signal updateCursorPosition
    property string fileName: ""
    property int lineNumber: -1
    property int columnNumber: 0
    property int typeId: -1

    property bool selectionRangeMode: false

    property bool selectionRangeReady: selectionRange.ready
    property real selectionRangeStart: selectionRange.startTime
    property real selectionRangeEnd: selectionRange.startTime + selectionRange.duration

    color: "#dcdcdc"

    // ***** connections with external objects
    Connections {
        target: zoomControl
        onRangeChanged: {
            backgroundMarks.updateMarks(zoomControl.rangeStart, zoomControl.rangeEnd);
            zoomSliderToolBar.updateZoomLevel();
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
            zoomControl.setRange(zoomControl.traceStart,
                                 zoomControl.traceStart + zoomControl.traceDuration / 10);
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
        }
    }

    function clear() {
        flick.contentY = 0;
        flick.contentX = 0;
        flick.contentWidth = 0;
        view.clearData();
        rangeDetails.hide();
        selectionRangeMode = false;
        buttonsBar.updateRangeButton(selectionRangeMode);
        zoomSlider.externalUpdate = true;
        zoomSlider.value = zoomSlider.minimumValue;
        overview.clear();
        timeDisplay.clear();
    }

    function enableButtonsBar(enable) {
        buttonsBar.enabled = enable;
    }

    function recenter( centerPoint ) {
        var newStart = Math.floor(centerPoint - zoomControl.rangeDuration / 2);
        zoomControl.setRange(Math.max(newStart, zoomControl.traceStart),
                             Math.min(newStart + zoomControl.rangeDuration, zoomControl.traceEnd));
    }

    function recenterOnItem(modelIndex, itemIndex)
    {
        if (itemIndex === -1)
            return;

        // if item is outside of the view, jump back to its position
        if (qmlProfilerModelProxy.endTime(modelIndex, itemIndex) < zoomControl.rangeStart ||
                qmlProfilerModelProxy.startTime(modelIndex, itemIndex) > zoomControl.rangeEnd) {
            recenter((qmlProfilerModelProxy.startTime(modelIndex, itemIndex) +
                      qmlProfilerModelProxy.endTime(modelIndex, itemIndex)) / 2);
        }
        var row = qmlProfilerModelProxy.row(modelIndex, itemIndex);
        var totalRowOffset = qmlProfilerModelProxy.modelOffset(modelIndex) +
                qmlProfilerModelProxy.rowOffset(modelIndex, row);
        if (totalRowOffset > flick.contentY + flick.height ||
                totalRowOffset + qmlProfilerModelProxy.rowHeight(modelIndex, row) < flick.contentY)
            flick.contentY = Math.min(flick.contentHeight - flick.height,
                                      Math.max(0, totalRowOffset - flick.height / 2));
    }

    function selectByTypeId(typeId)
    {
        if (lockItemSelection || typeId === -1)
            return;

        lockItemSelection = true;

        var itemIndex = -1;
        var modelIndex = -1;
        var notes = qmlProfilerModelProxy.notesByTypeId(typeId);
        if (notes.length !== 0) {
            modelIndex = qmlProfilerModelProxy.noteTimelineModel(notes[0]);
            itemIndex = qmlProfilerModelProxy.noteTimelineIndex(notes[0]);
        } else {
            for (modelIndex = 0; modelIndex < qmlProfilerModelProxy.modelCount(); ++modelIndex) {
                if (modelIndex === view.selectedModel && view.selectedItem !== -1 &&
                        typeId === qmlProfilerModelProxy.typeId(modelIndex, view.selectedItem))
                    break;

                if (!qmlProfilerModelProxy.handlesTypeId(modelIndex, typeId))
                    continue;

                itemIndex = view.nextItemFromTypeId(modelIndex, typeId);
                if (itemIndex !== -1)
                    break;
            }
        }

        if (itemIndex !== -1) {
            // select an item, lock to it, and recenter if necessary
            view.selectFromEventIndex(modelIndex, itemIndex); // triggers recentering
            view.selectionLocked = true;
        }

        lockItemSelection = false;
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
        contentWidth: zoomControl.windowDuration * width / Math.max(1, zoomControl.rangeDuration)
        flickableDirection: Flickable.HorizontalAndVerticalFlick
        boundsBehavior: Flickable.StopAtBounds
        clip:true

        // ScrollView will try to deinteractivate it. We don't want that
        // as the horizontal flickable is interactive, too. We do occasionally
        // switch to non-interactive ourselves, though.
        property bool stayInteractive: true
        onInteractiveChanged: interactive = stayInteractive
        onStayInteractiveChanged: interactive = stayInteractive

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
            zoomer: zoomControl

            x: flick.contentX
            y: flick.contentY

            // paint "under" the vertical scrollbar, so that it always matches with the timemarks
            width: scroller.width
            height: flick.height

            onYChanged: requestPaint()
            onHeightChanged: requestPaint()
            property bool recursionGuard: false

            property int intX: x
            property int intWidth: width
            onIntXChanged: {
                if (recursionGuard)
                    return;

                recursionGuard = true;

                var newStartTime = intX * zoomControl.rangeDuration / intWidth +
                        zoomControl.windowStart;
                if (Math.abs(newStartTime - zoomControl.rangeStart) >= 1) {
                    var newEndTime = (intX + intWidth) * zoomControl.rangeDuration / intWidth +
                            zoomControl.windowStart;
                    zoomControl.setRange(newStartTime, newEndTime);
                }
                recursionGuard = false;
            }

            function updateWindow() {
                if (recursionGuard || zoomControl.rangeDuration <= 0)
                    return;

                recursionGuard = true;

                // This triggers an unwanted automatic change in contentX. We ignore that by
                // checking recursionGuard in this function and in updateZoomControl.
                flick.contentWidth = zoomControl.windowDuration * intWidth /
                        zoomControl.rangeDuration;

                var newStartX = (zoomControl.rangeStart - zoomControl.windowStart) * intWidth /
                        zoomControl.rangeDuration;

                if (isFinite(newStartX) && Math.abs(newStartX - flick.contentX) >= 1)
                    flick.contentX = newStartX;

                recursionGuard = false;
            }

            onSelectionChanged: {
                if (selectedItem !== -1) {
                    // display details
                    rangeDetails.showInfo(selectedModel, selectedItem);

                    // center view (horizontally)
                    recenterOnItem(selectedModel, selectedItem);
                    if (!lockItemSelection) {
                        lockItemSelection = true;
                        // update in other views
                        var eventLocation = qmlProfilerModelProxy.location(view.selectedModel,
                                                                           view.selectedItem);
                        gotoSourceLocation(eventLocation.file, eventLocation.line,
                                           eventLocation.column);
                        root.typeId = qmlProfilerModelProxy.typeId(view.selectedModel,
                                                                   view.selectedItem);
                        root.updateCursorPosition();
                        lockItemSelection = false;
                    }
                } else {
                    rangeDetails.hide();
                }
            }

            onItemPressed: {
                var location = qmlProfilerModelProxy.location(modelIndex, pressedItem);
                if (location.hasOwnProperty("file")) // not empty
                    root.gotoSourceLocation(location.file, location.line, location.column);
                root.typeId = qmlProfilerModelProxy.typeId(modelIndex, pressedItem);
                root.updateCursorPosition();
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
            zoomSlider.value = Math.pow(zoomControl.rangeDuration /
                                        Math.max(1, zoomControl.windowDuration),
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
                if (externalUpdate || zoomControl.windowEnd <= zoomControl.windowStart) {
                    // Zoom range is independently updated. We shouldn't mess
                    // with it here as otherwise we might introduce rounding
                    // or arithmetic errors.
                    externalUpdate = false;
                    return;
                }

                var windowLength = Math.max(
                            Math.pow(value / maximumValue, exponent) * zoomControl.windowDuration,
                            minWindowLength);

                var fixedPoint = (zoomControl.rangeStart + zoomControl.rangeEnd) / 2;
                if (view.selectedItem !== -1) {
                    // center on selected item if it's inside the current screen
                    var newFixedPoint = qmlProfilerModelProxy.startTime(view.selectedModel, view.selectedItem);
                    if (newFixedPoint >= zoomControl.rangeStart &&
                            newFixedPoint < zoomControl.rangeEnd)
                        fixedPoint = newFixedPoint;
                }

                var startTime = Math.max(zoomControl.windowStart, fixedPoint - windowLength / 2)
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
