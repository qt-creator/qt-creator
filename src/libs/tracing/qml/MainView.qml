// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls

import QtCreator.Tracing

Rectangle {
    id: root

    // ***** properties

    property bool lockItemSelection : false

    property string fileName: ""
    property int lineNumber: -1
    property int columnNumber: 0
    property int selectedModel: -1
    property int selectedItem: -1

    property bool selectionRangeMode: false
    property bool selectionRangeReady: selectionRange.ready
    property int typeId: content.typeId
    onWidthChanged: zoomSliderToolBar.updateZoomLevel();

    color: Theme.color(Theme.Timeline_BackgroundColor1)

    // ***** connections with external objects
    Connections {
        target: zoomControl
        function onRangeChanged() {
            zoomSliderToolBar.updateZoomLevel();
            content.scroll();
            selectionRange.update();
        }
        function onWindowChanged() {
            content.scroll();
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
        zoomSlider.value = zoomSlider.from;
    }

    // This is called from outside to synchronize the timeline to other views
    function selectByTypeId(typeId)
    {
        if (lockItemSelection || typeId === -1 || content.typeId === typeId)
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

            // set this here, so that propagateSelection doesn't trigger updateCursorPosition()
            content.typeId = typeId;
            content.select(modelIndex, itemIndex);
            content.selectionLocked = true;
        }
    }

    // This is called from outside to synchronize the timeline to other views
    function selectByIndices(modelIndex, eventIndex)
    {
        if (modelIndex >= 0 && modelIndex < timelineModelAggregator.models.length &&
                selectedItem !== -1) {
            // set this here, so that propagateSelection doesn't trigger updateCursorPosition()
            content.typeId = timelineModelAggregator.models[modelIndex].typeId(eventIndex);
        }
        content.select(modelIndex, eventIndex);
    }

    focus: true
    property bool shiftPressed: false;
    Keys.onPressed: (event) => {
        shiftPressed = (event.key === Qt.Key_Shift);
    }
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
        color: Theme.color(Theme.PanelStatusBarBackgroundColor)
        modelProxy: timelineModelAggregator
        zoomer: zoomControl
        reverseSelect: root.shiftPressed

        onMoveCategories: (sourceIndex, targetIndex) => {
            content.moveCategories(sourceIndex, targetIndex)
        }
        onSelectItem: (modelIndex, eventIndex) => {
            content.select(modelIndex, eventIndex)
        }
    }

    TimeDisplay {
        id: timeDisplay
        anchors.top: parent.top
        anchors.left: buttonsBar.right
        anchors.right: parent.right
        anchors.bottom: overview.top
        windowStart: zoomControl.windowStart
        rangeDuration: zoomControl.rangeDuration
        contentX: content.contentX
    }

    ButtonsBar {
        id: buttonsBar
        enabled: zoomControl.traceDuration > 0
        anchors.top: parent.top
        anchors.left: parent.left
        width: 150
        height: Theme.toolBarHeight()
        onZoomControlChanged: zoomSliderToolBar.visible = !zoomSliderToolBar.visible
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

        onWidthChanged: selectionRange.update();

        onPropagateSelection: (newModel, newItem) => {
            if (lockItemSelection || (newModel === selectedModel && newItem === selectedItem))
                return;

            lockItemSelection = true;
            if (selectedModel !== -1 && selectedModel !== newModel)
                select(selectedModel, -1);

            rangeDetails.saveNote();
            selectedItem = newItem
            selectedModel = newModel
            if (selectedItem !== -1) {
                // display details
                rangeDetails.showInfo();

                // update in other views
                var model = timelineModelAggregator.models[selectedModel];
                var eventLocation = model.location(selectedItem);
                if (eventLocation.file !== undefined) {
                    root.fileName = eventLocation.file;
                    root.lineNumber = eventLocation.line;
                    root.columnNumber = eventLocation.column;
                }
                var newTypeId = model.typeId(selectedItem);
                if (newTypeId !== typeId) {
                    typeId = newTypeId;
                    timelineModelAggregator.updateCursorPosition();
                }
            } else {
                selectedModel = -1;
                rangeDetails.hide();
            }
            lockItemSelection = false;
        }
    }

    MouseArea {
        id: selectionRangeControl
        enabled: root.selectionRangeMode &&
                 selectionRange.creationState !== selectionRange.creationFinished
        anchors.right: content.right
        anchors.left: buttonsBar.right
        anchors.top: content.top
        anchors.bottom: content.bottom
        hoverEnabled: enabled
        z: 2

        function handlePress() {
            if (selectionRange.creationState === selectionRange.creationFirstLimit) {
                content.interactive = false;
                selectionRange.setPos(selectionRangeControl.mouseX + content.contentX);
                selectionRange.creationState = selectionRange.creationSecondLimit;
            }
        }

        onReleased:  {
            if (selectionRange.creationState === selectionRange.creationSecondLimit) {
                content.interactive = true;
                selectionRange.creationState = selectionRange.creationFinished;
            }
        }
        onPressed: handlePress()
        onPositionChanged: {
            if (selectionRange.creationState === selectionRange.creationInactive)
                selectionRange.creationState = selectionRange.creationFirstLimit;

            if (selectionRangeControl.pressed ||
                    selectionRange.creationState !== selectionRange.creationFinished)
                selectionRange.setPos(selectionRangeControl.mouseX + content.contentX);
        }
        onCanceled: handlePress()
    }

    Flickable {
        flickableDirection: Flickable.HorizontalFlick
        clip: true
        interactive: false
        x: content.x
        y: content.y
        height: (root.selectionRangeMode &&
                 selectionRange.creationState !== selectionRange.creationInactive) ?
                    content.height : 0
        width: content.width
        contentX: content.contentX
        contentWidth: content.contentWidth

        SelectionRange {
            id: selectionRange
            zoomer: zoomControl

            onRangeDoubleClicked: {
                var diff = zoomer.minimumRangeLength - zoomer.selectionDuration;
                if (diff > 0)
                    zoomControl.setRange(zoomer.selectionStart - diff / 2,
                                         zoomer.selectionEnd + diff / 2);
                else
                    zoomControl.setRange(zoomer.selectionStart, zoomer.selectionEnd);
                root.selectionRangeMode = false;
            }

            function update() {
                // If you select something in the main view and then resize the current range by some
                // other means, the selection should stay where it was.
                var oldTimePerPixel = viewTimePerPixel;
                viewTimePerPixel = zoomControl.rangeDuration / content.width;
                if (creationState === creationFinished && oldTimePerPixel != viewTimePerPixel) {
                    var newWidth = rangeWidth * oldTimePerPixel / viewTimePerPixel;
                    rangeLeft = rangeLeft * oldTimePerPixel / viewTimePerPixel;
                    rangeRight = rangeLeft + newWidth;
                }
            }

        }
    }

    TimelineRulers {
        contentX: buttonsBar.width + content.contentX
        anchors.left: buttonsBar.right
        anchors.right: parent.right
        anchors.top: parent.top
        height: content.height + buttonsBar.height
        windowStart: zoomControl.windowStart
        viewTimePerPixel: selectionRange.viewTimePerPixel
        scaleHeight: buttonsBar.height
    }

    SelectionRangeDetails {
        z: 3
        x: 200
        y: 125

        clip: true
        id: selectionRangeDetails
        startTime: zoomControl.selectionStart
        duration: zoomControl.selectionDuration
        endTime: zoomControl.selectionEnd
        referenceDuration: zoomControl.rangeDuration
        showDuration: selectionRange.rangeWidth > 1
        hasContents: root.selectionRangeMode &&
                     selectionRange.creationState !== selectionRange.creationInactive

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
        x: 200
        y: 25

        noteReadonly: false
        clip: true
        locked: content.selectionLocked

        onRecenterOnItem: {
            content.select(root.selectedModel, root.selectedItem)
        }

        onLockedChanged: {
            content.selectionLocked = locked;
        }

        onUpdateNote: (text) => {
            if (timelineModelAggregator.notes && root.selectedModel != -1
                && root.selectedItem != -1) {
                timelineModelAggregator.notes.setText(
                            timelineModelAggregator.models[root.selectedModel].modelId,
                            root.selectedItem, text);
            }
        }

        function hide() {
            model = [];
            file = "";
            line = -1;
            column = 0;
            noteText = "";
            dialogTitle = "";
        }

        function saveNote() {
            noteFocus = false;
        }

        function showInfo() {
            var timelineModel = timelineModelAggregator.models[selectedModel];
            var eventData = timelineModel.orderedDetails(selectedItem)
            dialogTitle = eventData["title"] || "";
            rangeDetails.model = eventData["content"] || [];

            var location = timelineModel.location(selectedItem)
            if (location.hasOwnProperty("file")) { // not empty
                rangeDetails.file = location.file;
                rangeDetails.line = location.line;
                rangeDetails.column = location.column;
            } else {
                // reset to default values
                rangeDetails.file = "";
                rangeDetails.line = 0;
                rangeDetails.column = -1;
            }

            var notes = timelineModelAggregator.notes;
            var noteId = notes ? notes.get(timelineModel.modelId, selectedItem) : -1;
            rangeDetails.noteText = (noteId !== -1) ? notes.text(noteId) : "";
        }
    }

    Rectangle {
        id: zoomSliderToolBar
        objectName: "zoomSliderToolBar"
        color: Theme.color(Theme.Timeline_PanelBackgroundColor)
        enabled: buttonsBar.enabled
        visible: false
        width: buttonsBar.width
        height: buttonsBar.height
        anchors.left: parent.left
        anchors.top: buttonsBar.bottom

        function updateZoomLevel() {
            var newValue = Math.round(Math.pow(zoomControl.rangeDuration /
                                               Math.max(1, zoomControl.windowDuration),
                                               1 / zoomSlider.exponent) * zoomSlider.maximumValue);
            if (newValue !== zoomSlider.value) {
                zoomSlider.externalUpdate = true;
                zoomSlider.value = newValue;
            }
        }

        Slider {
            id: zoomSlider
            anchors.fill: parent
            from: 1
            to: 10000
            stepSize: 100

            property int exponent: 3
            property bool externalUpdate: false
            property int minWindowLength: 1e5 // 0.1 ms
            property double fixedPoint: 0
            onPressedChanged: fixedPoint = (zoomControl.rangeStart + zoomControl.rangeEnd) / 2;

            onValueChanged: {
                if (externalUpdate || zoomControl.windowEnd <= zoomControl.windowStart) {
                    // Zoom range is independently updated. We shouldn't mess
                    // with it here as otherwise we might introduce rounding
                    // or arithmetic errors.
                    externalUpdate = false;
                    return;
                }

                var windowLength = Math.max(
                            Math.pow(value / to, exponent) * zoomControl.windowDuration,
                            minWindowLength);

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
