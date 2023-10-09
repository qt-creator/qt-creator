// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQml.Models
import QtCreator.Tracing

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
    property bool isDragging: false

    Column {
        id: categoryContent
        anchors.left: parent.left
        anchors.right: parent.right

        DelegateModel {
            id: labelsModel

            // As we cannot retrieve items by visible index we keep an array of row counts here,
            // for the time marks to draw the row backgrounds in the right colors.
            property var rowCounts: new Array(categories.modelProxy.models.length)

            function updateRowCount(visualIndex, rowCount) {
                if (rowCounts[visualIndex] !== rowCount) {
                    rowCounts[visualIndex] = rowCount;
                    // Array don't "change" if entries change. We have to signal manually.
                    rowCountsChanged();
                }
            }

            model: categories.modelProxy.models
            delegate: Loader {
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

                    Rectangle {
                        height: loader.height
                        width: 3
                        color: modelData.categoryColor
                    }

                    CategoryLabel {
                        id: label
                        model: modelData
                        notesModel: categories.modelProxy.notes
                        visualIndex: loader.visualIndex
                        isDragging: categories.isDragging
                        reverseSelect: categories.reverseSelect
                        onDragStarted: categories.isDragging = true
                        onDragStopped: categories.isDragging = false
                        draggerParent: categories
                        contentY: categories.contentY
                        contentHeight: categories.contentHeight
                        visibleHeight: categories.height
                        width: 150
                        height: parent.height
                        dragOffset: loader.y

                        onDropped: (sourceIndex, targetIndex) => {
                            categories.moveCategories(sourceIndex, targetIndex);
                            labelsModel.items.move(sourceIndex, targetIndex);
                        }

                        onSelectById: (eventId) => {
                            categories.selectItem(index, eventId)
                        }

                        onSelectNextBySelectionId: (selectionId) => {
                            categories.selectItem(index, modelData.nextItemBySelectionId(
                                    selectionId, zoomer.rangeStart,
                                    categories.selectedModel === index ? categories.selectedItem :
                                                                         -1));
                        }

                        onSelectPrevBySelectionId: (selectionId) => {
                            categories.selectItem(index,  modelData.prevItemBySelectionId(
                                    selectionId, zoomer.rangeStart,
                                    categories.selectedModel === index ? categories.selectedItem :
                                                                         -1));
                        }
                    }

                    TimeMarks {
                        id: timeMarks
                        model: modelData
                        mockup: categories.modelProxy.height === 0
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
