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
import QtQuick.Controls 1.0

Rectangle {
    id: root

    // ***** properties

    property bool lockItemSelection : false

    signal updateCursorPosition
    property string fileName: ""
    property int lineNumber: -1
    property int columnNumber: 0
    property int selectedModel: -1
    property int selectedItem: -1

    property bool selectionRangeMode: false
    property bool selectionRangeReady: selectionRange.ready
    property int typeId: content.typeId

    onTypeIdChanged: updateCursorPosition()

    color: "#dcdcdc"

    // ***** connections with external objects
    Connections {
        target: zoomControl
        onRangeChanged: {
            zoomSliderToolBar.updateZoomLevel();
            content.scroll();

            // If you select something in the main view and then resize the current range by some
            // other means, the selection should stay where it was.
            var oldTimePerPixel = selectionRange.viewTimePerPixel;
            selectionRange.viewTimePerPixel = zoomControl.rangeDuration / content.width;
            if (selectionRange.creationState === selectionRange.creationFinished &&
                    oldTimePerPixel != selectionRange.viewTimePerPixel) {
                var newWidth = selectionRange.rangeWidth * oldTimePerPixel /
                        selectionRange.viewTimePerPixel;
                selectionRange.rangeLeft = selectionRange.rangeLeft * oldTimePerPixel /
                        selectionRange.viewTimePerPixel;
                selectionRange.rangeRight = selectionRange.rangeLeft + newWidth;
            }
        }
        onWindowChanged: {
            content.scroll();
        }
    }


    Connections {
        target: timelineModelAggregator
        onDataAvailable: {
            content.clearChildren();
            zoomControl.setRange(zoomControl.traceStart,
                                 zoomControl.traceStart + zoomControl.traceDuration / 10);
        }
    }

    onSelectionRangeModeChanged: {
        selectionRange.reset();
        buttonsBar.updateRangeButton(selectionRangeMode);
    }

    // ***** functions
    function clear() {
        content.clearChildren();
        rangeDetails.hide();
        selectionRangeMode = false;
        zoomSlider.externalUpdate = true;
        zoomSlider.value = zoomSlider.minimumValue;
    }

    function selectByTypeId(typeId)
    {
        if (lockItemSelection || typeId === -1)
            return;

        var itemIndex = -1;
        var modelIndex = -1;

        var notesModel = timelineModelAggregator.notes;
        var notes = notesModel ? notesModel.byTypeId(typeId) : [];
        if (notes.length !== 0) {
            itemIndex = notesModel.timelineIndex(notes[0]);
            var modelId = notesModel.timelineModel(notes[0]);
            for (modelIndex = 0; modelIndex < timelineModelAggregator.models.length;
                 ++modelIndex) {
                if (timelineModelAggregator.models[modelIndex].modelId === modelId)
                    break;
            }
        } else {
            for (modelIndex = 0; modelIndex < timelineModelAggregator.models.length; ++modelIndex) {
                if (modelIndex === selectedModel && selectedItem !== -1 &&
                        typeId === timelineModelAggregator.models[modelIndex].typeId(selectedItem))
                    break;

                if (!timelineModelAggregator.models[modelIndex].handlesTypeId(typeId))
                    continue;

                itemIndex = timelineModelAggregator.models[modelIndex].nextItemByTypeId(typeId,
                        zoomControl.rangeStart, selectedItem);
                if (itemIndex !== -1)
                    break;
            }
        }

        if (modelIndex !== -1 && modelIndex < timelineModelAggregator.models.length &&
                itemIndex !== -1) {
            // select an item, lock to it, and recenter if necessary
            content.select(modelIndex, itemIndex);
            content.selectionLocked = true;
        }
    }

    function selectByIndices(modelIndex, eventIndex)
    {
        content.select(modelIndex, eventIndex);
    }

    focus: true
    property bool shiftPressed: false;
    Keys.onPressed: shiftPressed = (event.key === Qt.Key_Shift);
    Keys.onReleased: shiftPressed = false;

    TimelineLabels {
        id: categories
        anchors.top: buttonsBar.bottom
        anchors.bottom: overview.top
        anchors.left: parent.left
        anchors.right: parent.right
        contentY: content.contentY
        selectedModel: root.selectedModel
        selectedItem: root.selectedItem
        color: root.color
        modelProxy: timelineModelAggregator
        zoomer: zoomControl
        reverseSelect: shiftPressed

        onMoveCategories: content.moveCategories(sourceIndex, targetIndex)
        onSelectItem: content.select(modelIndex, eventIndex)
    }

    TimeDisplay {
        id: timeDisplay
        anchors.top: parent.top
        anchors.left: buttonsBar.right
        anchors.right: parent.right
        anchors.bottom: overview.top
        windowStart: zoomControl.windowStart
        rangeDuration: Math.max(1, Math.round(zoomControl.rangeDuration))
        contentX: content.contentX
        clip: true
    }

    ButtonsBar {
        id: buttonsBar
        enabled: timelineModelAggregator.height > 0
        anchors.top: parent.top
        anchors.left: parent.left
        width: 150
        height: 24
        onZoomControlChanged: zoomSliderToolBar.visible = !zoomSliderToolBar.visible
        onFilterMenuChanged: filterMenu.visible = !filterMenu.visible
        onJumpToNext: {
            var next = timelineModelAggregator.nextItem(root.selectedModel, root.selectedItem,
                                                      zoomControl.rangeStart);
            content.select(next.model, next.item);
        }
        onJumpToPrev: {
            var prev = timelineModelAggregator.prevItem(root.selectedModel, root.selectedItem,
                                                      zoomControl.rangeEnd);
            content.select(prev.model, prev.item);
        }

        onRangeSelectChanged: selectionRangeMode = rangeButtonChecked();
        onLockChanged: content.selectionLocked = !lockButtonChecked();
    }

    TimelineContent {
        id: content
        anchors.left: buttonsBar.right
        anchors.top: buttonsBar.bottom
        anchors.bottom: overview.top
        anchors.right: parent.right
        selectionLocked: true
        zoomer: zoomControl
        modelProxy: timelineModelAggregator

        onSelectionLockedChanged: {
            buttonsBar.updateLockButton(selectionLocked);
        }

        onPropagateSelection: {
            if (lockItemSelection || (newModel === selectedModel && newItem === selectedItem))
                return;

            lockItemSelection = true;
            if (selectedModel !== -1 && selectedModel !== newModel)
                select(selectedModel, -1);

            selectedItem = newItem
            selectedModel = newModel
            if (selectedItem !== -1) {
                // display details
                rangeDetails.showInfo(selectedModel, selectedItem);

                // update in other views
                var model = timelineModelAggregator.models[selectedModel];
                var eventLocation = model.location(selectedItem);
                gotoSourceLocation(eventLocation.file, eventLocation.line,
                                   eventLocation.column);
                typeId = model.typeId(selectedItem);
            } else {
                rangeDetails.hide();
            }
            lockItemSelection = false;
        }

        onGotoSourceLocation: {
            if (file !== undefined) {
                root.fileName = file;
                root.lineNumber = line;
                root.columnNumber = column;
            }
        }
    }

    MouseArea {
        id: selectionRangeControl
        enabled: selectionRangeMode &&
                 selectionRange.creationState !== selectionRange.creationFinished
        anchors.right: content.right
        anchors.left: buttonsBar.right
        anchors.top: content.top
        anchors.bottom: content.bottom
        hoverEnabled: enabled
        z: 2

        onReleased:  {
            if (selectionRange.creationState === selectionRange.creationSecondLimit) {
                content.stayInteractive = true;
                selectionRange.creationState = selectionRange.creationFinished;
            }
        }
        onPressed:  {
            if (selectionRange.creationState === selectionRange.creationFirstLimit) {
                content.stayInteractive = false;
                selectionRange.setPos(selectionRangeControl.mouseX + content.contentX);
                selectionRange.creationState = selectionRange.creationSecondLimit;
            }
        }
        onPositionChanged: {
            if (selectionRange.creationState === selectionRange.creationInactive)
                selectionRange.creationState = selectionRange.creationFirstLimit;

            if (selectionRangeControl.pressed ||
                    selectionRange.creationState !== selectionRange.creationFinished)
                selectionRange.setPos(selectionRangeControl.mouseX + content.contentX);
        }
        onCanceled: pressed()
    }

    Flickable {
        flickableDirection: Flickable.HorizontalFlick
        clip: true
        visible: selectionRangeMode &&
                 selectionRange.creationState !== selectionRange.creationInactive
        interactive: false
        x: content.x + content.flickableItem.x
        y: content.y + content.flickableItem.y
        height: content.flickableItem.height
        width: content.flickableItem.width
        contentX: content.contentX
        contentWidth: content.contentWidth

        SelectionRange {
            id: selectionRange
            zoomer: zoomControl
            visible: parent.visible

            onRangeDoubleClicked: {
                var diff = 500 - zoomer.selectionDuration;
                if (diff > 0)
                    zoomControl.setRange(zoomer.selectionStart - diff / 2,
                                         zoomer.selectionEnd + diff / 2);
                else
                    zoomControl.setRange(zoomer.selectionStart, zoomer.selectionEnd);
                root.selectionRangeMode = false;
            }

        }
    }

    SelectionRangeDetails {
        z: 3
        x: 200
        y: 125

        id: selectionRangeDetails
        visible: selectionRange.visible
        startTime: zoomControl.selectionStart
        duration: zoomControl.selectionDuration
        endTime: zoomControl.selectionEnd
        showDuration: selectionRange.rangeWidth > 1

        onRecenter: {
            if ((zoomControl.selectionStart < zoomControl.rangeStart) ^
                    (zoomControl.selectionEnd > zoomControl.rangeEnd)) {
                var center = (zoomControl.selectionStart + zoomControl.selectionEnd) / 2;
                var halfDuration = Math.max(zoomControl.selectionDuration,
                                            zoomControl.rangeDuration) / 2;
                zoomControl.setRange(center - halfDuration, center + halfDuration);
            }
        }

        onClose: selectionRangeMode = false;
    }

    RangeDetails {
        id: rangeDetails

        z: 3
        visible: false
        x: 200
        y: 25

        locked: content.selectionLocked
        models: timelineModelAggregator.models
        notes: timelineModelAggregator.notes
        onRecenterOnItem: {
            content.gotoSourceLocation(file, line, column);
            content.select(selectedModel, selectedItem)
        }
        onToggleSelectionLocked: {
            content.selectionLocked = !content.selectionLocked;
        }
        onClearSelection: {
            content.propagateSelection(-1, -1);
        }
    }

    Rectangle {
        anchors.left: buttonsBar.right
        anchors.bottom: overview.top
        anchors.top: parent.top
        width: 1
        color: "#B0B0B0"
    }

    Rectangle {
        id: filterMenu
        color: "#9b9b9b"
        enabled: buttonsBar.enabled
        visible: false
        width: buttonsBar.width
        anchors.left: parent.left
        anchors.top: buttonsBar.bottom
        height: timelineModelAggregator.models.length * buttonsBar.height

        Repeater {
            id: filterMenuInner
            model: timelineModelAggregator.models
            CheckBox {
                anchors.left: filterMenu.left
                anchors.right: filterMenu.right
                height: buttonsBar.height
                y: index * height
                text: modelData.displayName
                enabled: !modelData.empty
                checked: !modelData.empty && !modelData.hidden
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
        width: buttonsBar.width
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
                    var model = timelineModelAggregator.models[root.selectedModel]
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
        modelProxy: timelineModelAggregator
        zoomer: zoomControl
    }

    Rectangle {
        // Opal glass pane for visualizing the "disabled" state.
        anchors.fill: parent
        z: 10
        color: parent.color
        opacity: 0.5
        visible: !parent.enabled
    }
}
