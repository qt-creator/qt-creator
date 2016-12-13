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
import QtQml.Models 2.1
import TimelineTheme 1.0

Flickable {
    id: categories
    flickableDirection: Flickable.VerticalFlick
    interactive: false
    property color color

    property int selectedModel
    property int selectedItem
    property bool reverseSelect
    property QtObject modelProxy
    property QtObject zoomer

    signal selectItem(int modelIndex, int eventIndex)
    signal moveCategories(int sourceIndex, int targetIndex)

    // reserve some more space than needed to prevent weird effects when resizing
    contentHeight: categoryContent.height + height

    // Dispatch the cursor shape to all labels. When dragging the DropArea receiving
    // the drag events is not necessarily related to the MouseArea receiving the mouse
    // events, so we can't use the drag events to determine the cursor shape.
    property bool dragging: false

    Column {
        id: categoryContent
        anchors.left: parent.left
        anchors.right: parent.right

        DelegateModel {
            id: labelsModel

            // As we cannot retrieve items by visible index we keep an array of row counts here,
            // for the time marks to draw the row backgrounds in the right colors.
            property var rowCounts: new Array(modelProxy.models.length)

            function updateRowCount(visualIndex, rowCount) {
                if (rowCounts[visualIndex] !== rowCount) {
                    rowCounts[visualIndex] = rowCount;
                    // Array don't "change" if entries change. We have to signal manually.
                    rowCountsChanged();
                }
            }

            model: modelProxy.models
            delegate: SynchronousReloader {
                id: loader
                asynchronous: y < categories.contentY + categories.height &&
                              y + height > categories.contentY
                active: modelData !== null && zoomer !== null &&
                        (zoomer.traceDuration <= 0 || (!modelData.hidden && !modelData.empty))
                height: active ? Math.max(modelData.height, modelData.defaultRowHeight) : 0
                width: categories.width
                property int visualIndex: DelegateModel.itemsIndex

                sourceComponent: Rectangle {
                    color: categories.color
                    height: loader.height
                    width: loader.width

                    CategoryLabel {
                        id: label
                        model: modelData
                        notesModel: modelProxy.notes
                        visualIndex: loader.visualIndex
                        dragging: categories.dragging
                        reverseSelect: categories.reverseSelect
                        onDragStarted: categories.dragging = true
                        onDragStopped: categories.dragging = false
                        draggerParent: categories
                        width: 150
                        height: parent.height
                        dragOffset: loader.y

                        onDropped: {
                            categories.moveCategories(sourceIndex, targetIndex);
                            labelsModel.items.move(sourceIndex, targetIndex);
                        }

                        onSelectById: {
                            categories.selectItem(index, eventId)
                        }

                        onSelectNextBySelectionId: {
                            categories.selectItem(index, modelData.nextItemBySelectionId(
                                    selectionId, zoomer.rangeStart,
                                    categories.selectedModel === index ? categories.selectedItem :
                                                                         -1));
                        }

                        onSelectPrevBySelectionId: {
                            categories.selectItem(index,  modelData.prevItemBySelectionId(
                                    selectionId, zoomer.rangeStart,
                                    categories.selectedModel === index ? categories.selectedItem :
                                                                         -1));
                        }
                    }

                    TimeMarks {
                        id: timeMarks
                        model: modelData
                        mockup: modelProxy.height === 0
                        anchors.right: parent.right
                        anchors.left: label.right
                        anchors.top: parent.top
                        anchors.bottom: parent.bottom
                        property int visualIndex: loader.visualIndex

                        // Quite a mouthful, but works fine: Add up all the row counts up to the one
                        // for this visual index and check if the result is even or odd.
                        startOdd: (labelsModel.rowCounts.slice(0, visualIndex).reduce(
                                       function(prev, rows) {return prev + rows}, 0) % 2) === 0

                        onRowCountChanged: labelsModel.updateRowCount(visualIndex, rowCount)
                        onVisualIndexChanged: labelsModel.updateRowCount(visualIndex, rowCount)
                    }

                    Rectangle {
                        opacity: loader.y === 0 ? 0 : 1
                        color: Theme.color(Theme.Timeline_DividerColor)
                        height: 1
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                    }
                }
            }
        }

        Repeater {
            model: labelsModel
        }
    }
}
