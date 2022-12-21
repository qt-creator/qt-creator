// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtCreator.Tracing

Rectangle {
    id: overview
    objectName: "Overview"
    color: Theme.color(Theme.Timeline_BackgroundColor2)

    property QtObject modelProxy
    property QtObject zoomer
    property bool recursionGuard: false
    onWidthChanged: updateRangeMover()

    function updateZoomer() {
        if (recursionGuard)
            return;
        recursionGuard = true;
        var newStartTime = rangeMover.rangeLeft * zoomer.traceDuration / width +
                zoomer.traceStart;
        var newEndTime = rangeMover.rangeRight * zoomer.traceDuration / width +
                zoomer.traceStart;
        if (isFinite(newStartTime) && isFinite(newEndTime) &&
                newEndTime - newStartTime > zoomer.minimumRangeLength)
            zoomer.setRange(newStartTime, newEndTime);
        recursionGuard = false;
    }

    function updateRangeMover() {
        if (recursionGuard)
            return;
        recursionGuard = true;
        var newRangeX = (zoomer.rangeStart - zoomer.traceStart) * width /
                zoomer.traceDuration;
        var newWidth = zoomer.rangeDuration * width / zoomer.traceDuration;
        var widthChanged = Math.abs(newWidth - rangeMover.rangeWidth) > 1;
        var leftChanged = Math.abs(newRangeX - rangeMover.rangeLeft) > 1;
        if (leftChanged)
            rangeMover.rangeLeft = newRangeX;

        if (leftChanged || widthChanged)
            rangeMover.rangeRight = newRangeX + newWidth;
        recursionGuard = false;
    }

    Connections {
        target: overview.zoomer
        function onRangeChanged() { overview.updateRangeMover(); }
    }

    TimeDisplay {
        id: timebar
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        textMargin: 2
        height: 10
        fontSize: 6
        labelsHeight: 10
        windowStart: overview.zoomer.traceStart
        alignedWindowStart: overview.zoomer.traceStart
        rangeDuration: overview.zoomer.traceDuration
        contentX: 0
        offsetX: 0
    }

    Column {
        anchors.top: timebar.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right

        id: renderArea

        Repeater {
            model: overview.modelProxy.models
            TimelineOverviewRenderer {
                model: modelData
                zoomer: overview.zoomer
                notes: overview.modelProxy.notes
                width: renderArea.width
                height: renderArea.height / overview.modelProxy.models.length
            }
        }
    }

    Repeater {
        id: noteSigns
        property var modelsById: overview.modelProxy.models.reduce(function(prev, model) {
            prev[model.modelId] = model;
            return prev;
        }, {});

        property int vertSpace: renderArea.height / 7
        property color noteColor: Theme.color(Theme.Timeline_HighlightColor)
        readonly property double spacing: parent.width / overview.zoomer.traceDuration

        model: overview.modelProxy.notes ? overview.modelProxy.notes.count : 0
        Item {
            property int timelineIndex: overview.modelProxy.notes.timelineIndex(index)
            property int timelineModel: overview.modelProxy.notes.timelineModel(index)
            property double startTime: noteSigns.modelsById[timelineModel].startTime(timelineIndex)
            property double endTime: noteSigns.modelsById[timelineModel].endTime(timelineIndex)
            x: ((startTime + endTime) / 2 - overview.zoomer.traceStart) * noteSigns.spacing
            y: timebar.height + noteSigns.vertSpace
            height: noteSigns.vertSpace * 5
            width: 2
            Rectangle {
                color: noteSigns.noteColor
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                height: noteSigns.vertSpace * 3
            }

            Rectangle {
                color: noteSigns.noteColor
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                height: noteSigns.vertSpace
            }
        }
    }

    // ***** child items
    MouseArea {
        anchors.fill: parent
        function jumpTo(posX) {
            var newX = posX - rangeMover.rangeWidth / 2;
            if (newX < 0)
                newX = 0;
            if (newX + rangeMover.rangeWidth > overview.width)
                newX = overview.width - rangeMover.rangeWidth;

            if (newX < rangeMover.rangeLeft) {
                // Changing left border will change width, so precompute right border here.
                var right = newX + rangeMover.rangeWidth;
                rangeMover.rangeLeft = newX;
                rangeMover.rangeRight = right;
            } else if (newX > rangeMover.rangeLeft) {
                rangeMover.rangeRight = newX + rangeMover.rangeWidth;
                rangeMover.rangeLeft = newX;
            }
        }

        onPressed: (mouse) => {
            jumpTo(mouse.x);
        }
        onPositionChanged: (mouse) => {
            jumpTo(mouse.x);
        }
    }

    RangeMover {
        id: rangeMover
        visible: overview.modelProxy.height > 0
        onRangeLeftChanged: overview.updateZoomer()
        onRangeRightChanged: overview.updateZoomer()
    }
}
