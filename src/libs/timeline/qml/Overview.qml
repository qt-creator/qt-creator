/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

import QtQuick 2.1
import TimelineOverviewRenderer 1.0
import TimelineTheme 1.0

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
        target: zoomer
        onRangeChanged: updateRangeMover()
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
        windowStart: zoomer.traceStart
        alignedWindowStart: zoomer.traceStart
        rangeDuration: zoomer.traceDuration
        contentX: 0
        offsetX: 0
    }

    Column {
        anchors.top: timebar.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right

        id: column

        Repeater {
            model: modelProxy.models
            TimelineOverviewRenderer {
                model: modelData
                zoomer: overview.zoomer
                notes: modelProxy.notes
                width: column.width
                height: column.height / modelProxy.models.length
            }
        }
    }

    Repeater {
        id: noteSigns
        property var modelsById: modelProxy.models.reduce(function(prev, model) {
            prev[model.modelId] = model;
            return prev;
        }, {});

        property int vertSpace: column.height / 7
        property color noteColor: Theme.color(Theme.Timeline_HighlightColor)
        readonly property double spacing: parent.width / zoomer.traceDuration

        model: modelProxy.notes ? modelProxy.notes.count : 0
        Item {
            property int timelineIndex: modelProxy.notes.timelineIndex(index)
            property int timelineModel: modelProxy.notes.timelineModel(index)
            property double startTime: noteSigns.modelsById[timelineModel].startTime(timelineIndex)
            property double endTime: noteSigns.modelsById[timelineModel].endTime(timelineIndex)
            x: ((startTime + endTime) / 2 - zoomer.traceStart) * noteSigns.spacing
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

        onPressed: {
            jumpTo(mouse.x);
        }
        onPositionChanged: {
            jumpTo(mouse.x);
        }
    }

    RangeMover {
        id: rangeMover
        visible: modelProxy.height > 0
        onRangeLeftChanged: overview.updateZoomer()
        onRangeRightChanged: overview.updateZoomer()
    }
}
