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
    signal gotoSourceLocation(string file, int line, int column)

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

        // Update the zoom control on srolling.
        onContentXChanged: {
            if (recursionGuard)
                return;

            recursionGuard = true;

            var newStartTime = contentX * zoomer.rangeDuration / scroller.width +
                    zoomer.windowStart;
            if (isFinite(newStartTime) && Math.abs(newStartTime - zoomer.rangeStart) >= 1) {
                var newEndTime = (contentX + scroller.width) * zoomer.rangeDuration /
                        scroller.width + zoomer.windowStart;
                if (isFinite(newEndTime))
                    zoomer.setRange(newStartTime, newEndTime);
            }
            recursionGuard = false;
        }

        // Scroll when the zoom control is updated
        function scroll() {
            if (recursionGuard || zoomer.rangeDuration <= 0)
                return;
            recursionGuard = true;

            var newWidth = zoomer.windowDuration * scroller.width / zoomer.rangeDuration;
            if (isFinite(newWidth) && Math.abs(newWidth - contentWidth) >= 1)
                contentWidth = newWidth;

            var newStartX = (zoomer.rangeStart - zoomer.windowStart) * scroller.width /
                    zoomer.rangeDuration;
            if (isFinite(newStartX) && Math.abs(newStartX - contentX) >= 1)
                contentX = newStartX;

            recursionGuard = false;
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

                    Connections {
                        target: scroller
                        onSelectionLockedChanged: {
                            renderer.selectionLocked = scroller.selectionLocked;
                        }
                    }

                    onSelectionLockedChanged: {
                        scroller.selectionLocked = renderer.selectionLocked;
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

                    onSelectedItemChanged: {
                        scroller.propagateSelection(index, selectedItem);
                    }

                    onItemPressed: {
                        if (pressedItem === -1) {
                            // User clicked on empty space. Remove selection.
                            scroller.propagateSelection(-1, -1);
                        } else {
                            var location = model.location(pressedItem);
                            if (location.hasOwnProperty("file")) // not empty
                                scroller.gotoSourceLocation(location.file, location.line,
                                                            location.column);
                            scroller.typeId = model.typeId(pressedItem);
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
