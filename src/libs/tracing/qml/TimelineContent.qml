// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import QtCreator.Tracing
import QtQml.Models

Flickable {
    id: flick
    clip: true

    contentHeight: timelineView.height + height
    flickableDirection: Flickable.HorizontalAndVerticalFlick
    boundsBehavior: Flickable.StopAtBounds
    pixelAligned: true

    ScrollBar.horizontal: ScrollBar {}
    ScrollBar.vertical: ScrollBar {}

    property bool recursionGuard: false
    property QtObject zoomer
    property QtObject modelProxy

    property bool selectionLocked
    property int typeId

    signal propagateSelection(int newModel, int newItem)

    function clearChildren()
    {
        timelineView.clearChildren();
    }

    function select(modelIndex, eventIndex)
    {
        timelineView.select(modelIndex, eventIndex);
    }

    function moveCategories(sourceIndex, targetIndex)
    {
        timelineModel.items.move(sourceIndex, targetIndex)
    }


    function guarded(operation) {
        if (recursionGuard)
            return;
        recursionGuard = true;
        operation();
        recursionGuard = false;
    }

    // Logically we should bind to flick.width above as we use flick.width in scroll().
    // However, this width changes before flick.width when the window is resized and if we
    // don't explicitly set contentX here, for some reason an automatic change in contentX is
    // triggered after this width has changed, but before flick.width changes. This would be
    // indistinguishabe from a manual flick by the user and thus changes the range position. We
    // don't want to change the range position on resizing the window. Therefore we bind to this
    // width.
    onWidthChanged: scroll()

    // Update the zoom control on scrolling.
    onContentXChanged: guarded(function() {
        var newStartTime = contentX * zoomer.rangeDuration / width + zoomer.windowStart;
        if (isFinite(newStartTime) && Math.abs(newStartTime - zoomer.rangeStart) >= 1) {
            var newEndTime = (contentX + width) * zoomer.rangeDuration / width + zoomer.windowStart;
            if (isFinite(newEndTime))
                zoomer.setRange(newStartTime, newEndTime);
        }
    });

    // Scroll when the zoom control is updated
    function scroll() {
        guarded(function() {
            if (zoomer.rangeDuration <= 0) {
                contentWidth = 0;
                contentX = 0;
            } else {
                var newWidth = zoomer.windowDuration * width / zoomer.rangeDuration;
                if (isFinite(newWidth) && Math.abs(newWidth - contentWidth) >= 1)
                    contentWidth = newWidth;
                var newStartX = (zoomer.rangeStart - zoomer.windowStart) * width /
                        zoomer.rangeDuration;
                if (isFinite(newStartX) && Math.abs(newStartX - contentX) >= 1)
                    contentX = newStartX;
            }
        });
    }

    Column {
        id: timelineView

        signal clearChildren
        signal select(int modelIndex, int eventIndex)

        DelegateModel {
            id: timelineModel
            model: flick.modelProxy.models
            delegate: TimelineRenderer {
                id: renderer
                model: modelData
                notes: flick.modelProxy.notes
                zoomer: flick.zoomer
                selectionLocked: flick.selectionLocked
                x: 0
                height: modelData.height
                property int visualIndex: DelegateModel.itemsIndex

                // paint "under" the vertical scrollbar, so that it always matches with the
                // timemarks
                width: flick.contentWidth

                Connections {
                    target: timelineView
                    function onClearChildren() { renderer.clearData(); }
                    function onSelect(modelIndex, eventIndex) {
                        if (modelIndex === index || modelIndex === -1) {
                            renderer.selectedItem = eventIndex;
                            if (eventIndex !== -1)
                                renderer.recenter();
                        }
                    }
                }

                function recenter() {
                    if (modelData.endTime(renderer.selectedItem) < zoomer.rangeStart ||
                            modelData.startTime(renderer.selectedItem) > zoomer.rangeEnd) {

                        var newStart = Math.max((modelData.startTime(renderer.selectedItem) +
                                                 modelData.endTime(renderer.selectedItem) -
                                                 zoomer.rangeDuration) / 2, zoomer.traceStart);
                        zoomer.setRange(newStart,
                                        Math.min(newStart + zoomer.rangeDuration, zoomer.traceEnd));
                    }

                    var row = renderer.model.row(renderer.selectedItem);
                    var rowStart = modelData.rowOffset(row) + y;
                    var rowEnd = rowStart + modelData.rowHeight(row);
                    if (rowStart < flick.contentY || rowEnd - flick.height > flick.contentY)
                        flick.contentY = (rowStart + rowEnd - flick.height) / 2;
                }

                onSelectedItemChanged: flick.propagateSelection(index, renderer.selectedItem);

                Connections {
                    target: renderer.model
                    function onDetailsChanged() {
                        if (renderer.selectedItem != -1) {
                            flick.propagateSelection(-1, -1);
                            flick.propagateSelection(index, selectedItem);
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
}
