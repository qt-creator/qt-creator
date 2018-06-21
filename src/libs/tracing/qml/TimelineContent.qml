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

import QtQuick 2.0
import QtQuick.Controls 1.2
import TimelineRenderer 1.0
import QtQml.Models 2.1

ScrollView {
    id: scroller

    property QtObject zoomer
    property QtObject modelProxy

    property int contentX: flick.contentX
    property int contentY: flick.contentY
    property int contentWidth: flick.contentWidth
    property int contentHeight: flick.contentHeight
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

    function scroll()
    {
        flick.scroll();
    }

    // ScrollView will try to deinteractivate it. We don't want that
    // as the horizontal flickable is interactive, too. We do occasionally
    // switch to non-interactive ourselves, though.
    property bool stayInteractive: true
    onStayInteractiveChanged: flick.interactive = stayInteractive

    Flickable {
        id: flick
        contentHeight: timelineView.height + height
        flickableDirection: Flickable.HorizontalAndVerticalFlick
        boundsBehavior: Flickable.StopAtBounds
        pixelAligned: true

        onInteractiveChanged: interactive = stayInteractive

        property bool recursionGuard: false

        function guarded(operation) {
            if (recursionGuard)
                return;
            recursionGuard = true;
            operation();
            recursionGuard = false;
        }

        // Logically we should bind to scroller.width above as we use scroller.width in scroll().
        // However, this width changes before scroller.width when the window is resized and if we
        // don't explicitly set contentX here, for some reason an automatic change in contentX is
        // triggered after this width has changed, but before scroller.width changes. This would be
        // indistinguishabe from a manual flick by the user and thus changes the range position. We
        // don't want to change the range position on resizing the window. Therefore we bind to this
        // width.
        onWidthChanged: scroll()

        // Update the zoom control on scrolling.
        onContentXChanged: guarded(function() {
            var newStartTime = contentX * zoomer.rangeDuration / scroller.width
                    + zoomer.windowStart;
            if (isFinite(newStartTime) && Math.abs(newStartTime - zoomer.rangeStart) >= 1) {
                var newEndTime = (contentX + scroller.width) * zoomer.rangeDuration / scroller.width
                        + zoomer.windowStart;
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
                    var newWidth = zoomer.windowDuration * scroller.width / zoomer.rangeDuration;
                    if (isFinite(newWidth) && Math.abs(newWidth - contentWidth) >= 1)
                        contentWidth = newWidth;
                    var newStartX = (zoomer.rangeStart - zoomer.windowStart) * scroller.width /
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
                model: modelProxy.models
                delegate: TimelineRenderer {
                    id: renderer
                    model: modelData
                    notes: modelProxy.notes
                    zoomer: scroller.zoomer
                    selectionLocked: scroller.selectionLocked
                    x: 0
                    height: modelData.height
                    property int visualIndex: DelegateModel.itemsIndex

                    // paint "under" the vertical scrollbar, so that it always matches with the
                    // timemarks
                    width: flick.contentWidth

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

                    function recenter() {
                        if (modelData.endTime(selectedItem) < zoomer.rangeStart ||
                                modelData.startTime(selectedItem) > zoomer.rangeEnd) {

                            var newStart = Math.max((modelData.startTime(selectedItem) +
                                                     modelData.endTime(selectedItem) -
                                                     zoomer.rangeDuration) / 2, zoomer.traceStart);
                            zoomer.setRange(newStart,
                                            Math.min(newStart + zoomer.rangeDuration, zoomer.traceEnd));
                        }

                        var row = modelData.row(selectedItem);
                        var rowStart = modelData.rowOffset(row) + y;
                        var rowEnd = rowStart + modelData.rowHeight(row);
                        if (rowStart < flick.contentY || rowEnd - scroller.height > flick.contentY)
                            flick.contentY = (rowStart + rowEnd - scroller.height) / 2;
                    }

                    onSelectedItemChanged: scroller.propagateSelection(index, selectedItem);

                    Connections {
                        target: model
                        onDetailsChanged: {
                            if (selectedItem != -1) {
                                scroller.propagateSelection(-1, -1);
                                scroller.propagateSelection(index, selectedItem);
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
}
