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
import QtQml.Models 2.1

Rectangle {
    id: root

    // ***** properties

    property bool selectionLocked : true
    property bool lockItemSelection : false

    property real mainviewTimePerPixel: zoomControl.rangeDuration / root.width

    signal updateCursorPosition
    property string fileName: ""
    property int lineNumber: -1
    property int columnNumber: 0
    property int typeId: -1
    property int selectedModel: -1
    property int selectedItem: -1

    property bool selectionRangeMode: false

    property bool selectionRangeReady: selectionRange.ready
    property real selectionRangeStart: selectionRange.startTime
    property real selectionRangeEnd: selectionRange.startTime + selectionRange.duration

    color: "#dcdcdc"

    // ***** connections with external objects
    Connections {
        target: zoomControl
        onRangeChanged: {
            zoomSliderToolBar.updateZoomLevel();
            flick.updateWindow();
        }
        onWindowChanged: {
            flick.updateWindow();
        }
    }


    Connections {
        target: qmlProfilerModelProxy
        onDataAvailable: {
            timelineView.clearChildren();
            zoomControl.setRange(zoomControl.traceStart,
                                 zoomControl.traceStart + zoomControl.traceDuration / 10);
        }
    }


    // ***** functions
    function gotoSourceLocation(file,line,column) {
        if (file !== undefined) {
            root.fileName = file;
            root.lineNumber = line;
            root.columnNumber = column;
        }
    }

    function recenterOnItem() {
        timelineView.select(selectedModel, selectedItem);
    }

    function clear() {
        flick.contentY = 0;
        flick.contentX = 0;
        flick.contentWidth = 0;
        timelineView.clearChildren();
        rangeDetails.hide();
        selectionRangeMode = false;
        zoomSlider.externalUpdate = true;
        zoomSlider.value = zoomSlider.minimumValue;
        overview.clear();
        timeDisplay.clear();
    }

    function propagateSelection(newModel, newItem) {
        if (lockItemSelection || (newModel === selectedModel && newItem === selectedItem))
            return;

        lockItemSelection = true;
        if (selectedModel !== -1 && selectedModel !== newModel)
            timelineView.select(selectedModel, -1);

        selectedItem = newItem
        selectedModel = newModel
        if (selectedItem !== -1) {
            // display details
            rangeDetails.showInfo(selectedModel, selectedItem);

            // update in other views
            var model = qmlProfilerModelProxy.models[selectedModel];
            var eventLocation = model.location(selectedItem);
            gotoSourceLocation(eventLocation.file, eventLocation.line,
                               eventLocation.column);
            typeId = model.typeId(selectedItem);
            updateCursorPosition();
        } else {
            rangeDetails.hide();
        }
        lockItemSelection = false;
    }

    function enableButtonsBar(enable) {
        buttonsBar.enabled = enable;
    }

    function selectByTypeId(typeId)
    {
        if (lockItemSelection || typeId === -1)
            return;

        var itemIndex = -1;
        var modelIndex = -1;
        var notes = qmlProfilerModelProxy.notesByTypeId(typeId);
        if (notes.length !== 0) {
            modelIndex = qmlProfilerModelProxy.noteTimelineModel(notes[0]);
            itemIndex = qmlProfilerModelProxy.noteTimelineIndex(notes[0]);
        } else {
            for (modelIndex = 0; modelIndex < qmlProfilerModelProxy.models.length; ++modelIndex) {
                if (modelIndex === selectedModel && selectedItem !== -1 &&
                        typeId === qmlProfilerModelProxy.models[modelIndex].typeId(selectedItem))
                    break;

                if (!qmlProfilerModelProxy.models[modelIndex].handlesTypeId(typeId))
                    continue;

                itemIndex = qmlProfilerModelProxy.models[modelIndex].nextItemByTypeId(typeId,
                        zoomControl.rangeStart, selectedItem);
                if (itemIndex !== -1)
                    break;
            }
        }

        if (itemIndex !== -1) {
            // select an item, lock to it, and recenter if necessary
            timelineView.select(modelIndex, itemIndex);
            root.selectionLocked = true;
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

            Column {
                id: col

                // Dispatch the cursor shape to all labels. When dragging the DropArea receiving
                // the drag events is not necessarily related to the MouseArea receiving the mouse
                // events, so we can't use the drag events to determine the cursor shape.
                property bool dragging: false

                DelegateModel {
                    id: labelsModel
                    model: qmlProfilerModelProxy.models
                    delegate: CategoryLabel {
                        model: modelData
                        mockup: qmlProfilerModelProxy.height == 0
                        visualIndex: DelegateModel.itemsIndex
                        dragging: col.dragging
                        reverseSelect: root.shiftPressed
                        onDragStarted: col.dragging = true
                        onDragStopped: col.dragging = false
                        draggerParent: labels
                        dragOffset: y

                        onDropped: {
                            timelineModel.items.move(sourceIndex, targetIndex);
                            labelsModel.items.move(sourceIndex, targetIndex);
                        }

                        onSelectById: {
                            timelineView.select(index, eventId)
                        }

                        onSelectNextBySelectionId: {
                            timelineView.select(index, modelData.nextItemBySelectionId(selectionId,
                                    zoomControl.rangeStart,
                                    root.selectedModel === index ? root.selectedItem : -1));
                        }

                        onSelectPrevBySelectionId: {
                            timelineView.select(index, modelData.prevItemBySelectionId(selectionId,
                                    zoomControl.rangeStart,
                                    root.selectedModel === index ? root.selectedItem : -1));
                        }


                    }
                }

                Repeater {
                    model: labelsModel
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
        onJumpToNext: {
            var next = qmlProfilerModelProxy.nextItem(root.selectedModel, root.selectedItem,
                                                      zoomControl.rangeStart);
            timelineView.select(next.model, next.item);
        }
        onJumpToPrev: {
            var prev = qmlProfilerModelProxy.prevItem(root.selectedModel, root.selectedItem,
                                                      zoomControl.rangeEnd);
            timelineView.select(prev.model, prev.item);
        }

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

        property bool recursionGuard: false
        property int intX: contentX
        property int intWidth: scroller.width

        // Update the zoom control on srolling.
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

        // Scroll when the zoom control is updated
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

        // ***** child items
        SelectionRange {
            id: selectionRange
            visible: root.selectionRangeMode && creationState !== 0
            z: 2
        }

        Column {
            id: timelineView

            signal clearChildren
            signal select(int modelIndex, int eventIndex)

            // As we cannot retrieve items by visible index we keep an array of row counts here,
            // for the time marks to draw the row backgrounds in the right colors.
            property var rowCounts: new Array(qmlProfilerModelProxy.models.length)

            DelegateModel {
                id: timelineModel
                model: qmlProfilerModelProxy.models
                delegate: Item {
                    id: spacer
                    height: modelData.height
                    width: flick.contentWidth
                    property int rowCount: (modelData.empty || modelData.hidden) ?
                                               0 : modelData.rowCount
                    property int visualIndex: DelegateModel.itemsIndex

                    function updateRowParentCount() {
                        if (timelineView.rowCounts[visualIndex] !== rowCount) {
                            timelineView.rowCounts[visualIndex] = rowCount;
                            // Array don't "change" if entries change. We have to signal manually.
                            timelineView.rowCountsChanged();
                        }
                    }

                    onRowCountChanged: updateRowParentCount()
                    onVisualIndexChanged: updateRowParentCount()

                    TimeMarks {
                        model: modelData
                        id: backgroundMarks
                        anchors.fill: renderer
                        startTime: zoomControl.rangeStart
                        endTime: zoomControl.rangeEnd

                        // Quite a mouthful, but works fine: Add up all the row counts up to the one
                        // for this visual index and check if the result is even or odd.
                        startOdd: (timelineView.rowCounts.slice(0, spacer.visualIndex).reduce(
                                       function(prev, rows) {return prev + rows}, 0) % 2) === 0
                        onStartOddChanged: requestPaint()
                    }

                    TimelineRenderer {
                        id: renderer
                        model: modelData
                        notes: qmlProfilerModelProxy.notes
                        zoomer: zoomControl
                        selectionLocked: root.selectionLocked
                        x: flick.contentX

                        // paint "under" the vertical scrollbar, so that it always matches with the
                        // timemarks
                        width: scroller.width
                        property int yScrollStartDiff: flick.contentY - parent.y
                        property int yScrollEndDiff: flick.height - parent.height + yScrollStartDiff
                        y: Math.min(parent.height, Math.max(0, yScrollStartDiff))
                        height: {
                            if (yScrollStartDiff > 0) {
                                return Math.max(0, Math.min(flick.height,
                                                            parent.height - yScrollStartDiff));
                            } else if (yScrollEndDiff < 0) {
                                return Math.max(0, Math.min(flick.height,
                                                            parent.height + yScrollEndDiff));
                            } else {
                                return parent.height;
                            }
                        }

                        Connections {
                            target: timelineView
                            onClearChildren: renderer.clearData()
                            onSelect: {
                                if (modelIndex === index || modelIndex === -1) {
                                    renderer.selectedItem = eventIndex;
                                    if (eventIndex !== -1)
                                        renderer.recenter();
                                }
                            }
                        }

                        Connections {
                            target: root
                            onSelectionLockedChanged: {
                                renderer.selectionLocked = root.selectionLocked;
                            }
                        }

                        onSelectionLockedChanged: {
                            root.selectionLocked = renderer.selectionLocked;
                        }

                        function recenter() {
                            if (modelData.endTime(selectedItem) < zoomer.rangeStart ||
                                    modelData.startTime(selectedItem) > zoomer.rangeEnd) {

                                var newStart = (modelData.startTime(selectedItem) +
                                                modelData.endTime(selectedItem) -
                                                zoomer.rangeDuration) / 2;
                                zoomer.setRange(Math.max(newStart, zoomer.traceStart),
                                                Math.min(newStart + zoomer.rangeDuration,
                                                         zoomer.traceEnd));
                            }

                            if (spacer.y + spacer.height < flick.contentY)
                                flick.contentY = spacer.y + spacer.height;
                            else if (spacer.y - flick.height > flick.contentY)
                                flick.contentY = spacer.y - flick.height;

                            var row = modelData.row(selectedItem);
                            var rowStart = modelData.rowOffset(row);
                            var rowEnd = rowStart + modelData.rowHeight(row);
                            if (rowStart < y)
                                flick.contentY -= y - rowStart;
                            else if (rowEnd > y + height)
                                flick.contentY += rowEnd - y - height;
                        }

                        onSelectedItemChanged: {
                            root.propagateSelection(index, selectedItem);
                        }

                        onItemPressed: {
                            if (pressedItem === -1) {
                                // User clicked on empty space. Remove selection.
                                root.propagateSelection(-1, -1);
                            } else {
                                var location = model.location(pressedItem);
                                if (location.hasOwnProperty("file")) // not empty
                                    root.gotoSourceLocation(location.file, location.line, location.column);
                                root.typeId = model.typeId(pressedItem);
                                root.updateCursorPosition();
                            }
                        }
                    }
                }
            }

            Repeater {
                id: repeater
                model: timelineModel
            }
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
        property alias locked: root.selectionLocked
        models: qmlProfilerModelProxy.models
        notes: qmlProfilerModelProxy.notes
    }

    Rectangle {
        id: filterMenu
        color: "#9b9b9b"
        enabled: buttonsBar.enabled
        visible: false
        width: labels.width
        anchors.left: parent.left
        anchors.top: buttonsBar.bottom
        height: qmlProfilerModelProxy.models.length * buttonsBar.height

        Repeater {
            id: filterMenuInner
            model: qmlProfilerModelProxy.models
            CheckBox {
                anchors.left: filterMenu.left
                anchors.right: filterMenu.right
                height: buttonsBar.height
                y: index * height
                text: modelData.displayName
                enabled: !modelData.empty
                checked: enabled && !modelData.hidden
                onCheckedChanged: modelData.hidden = !checked
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
                if (root.selectedItem !== -1) {
                    // center on selected item if it's inside the current screen
                    var model = qmlProfilerModelProxy.models[root.selectedModel]
                    var newFixedPoint = (model.startTime(root.selectedItem) +
                                         model.endTime(root.selectedItem)) / 2;
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
