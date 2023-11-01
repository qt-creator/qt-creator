// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls

import QtCreator.Tracing

Item {
    id: labelContainer

    property TimelineModel model
    property QtObject notesModel
    property string text: model ? model.displayName : ""
    property bool expanded: model && model.expanded
    property var labels: (expanded && model) ? model.labels : []

    property bool isDragging
    property int visualIndex
    property int dragOffset
    property Item draggerParent
    property real contentY: 0
    property real contentHeight: draggerParent.height
    property real visibleHeight: contentHeight
    property int contentBottom: contentY + visibleHeight - dragOffset

    signal dragStarted;
    signal dragStopped;
    signal dropped(int sourceIndex, int targetIndex)

    signal selectById(int eventId)
    signal selectNextBySelectionId(int selectionId)
    signal selectPrevBySelectionId(int selectionId)

    property bool reverseSelect: false

    MouseArea {
        id: dragArea
        anchors.fill: txt
        drag.target: dragger
        cursorShape: labelContainer.isDragging ? Qt.ClosedHandCursor : Qt.OpenHandCursor
        // Account for parent change below
        drag.minimumY: labelContainer.isDragging ? 0 : -labelContainer.dragOffset
        drag.maximumY: labelContainer.visibleHeight - (labelContainer.isDragging ? 0 : labelContainer.dragOffset)
        drag.axis: Drag.YAxis
        hoverEnabled: true
        ToolTip {
            text: labelContainer.model.tooltip || labelContainer.text
            visible: enabled && parent.containsMouse
            delay: 1000
        }
    }

    DropArea {
        id: dropArea

        onPositionChanged: (drag) => {
            var sourceIndex = drag.source.visualIndex;
            if (drag.source.y === 0) {
                // special case for first position: Always swap, no matter if upper border touched.
                if (sourceIndex > visualIndex)
                    labelContainer.dropped(sourceIndex, visualIndex);
            } else if (sourceIndex !== visualIndex && sourceIndex !== visualIndex + 1) {
                labelContainer.dropped(sourceIndex, sourceIndex > visualIndex ? visualIndex + 1 :
                                                                                visualIndex);
            }
        }

        anchors.fill: parent
    }

    TimelineText {
        id: txt
        anchors.left: parent.left
        anchors.leftMargin: 5
        anchors.right: notesButton.visible ? notesButton.left : notesButton.right

        text: labelContainer.text
        color: Theme.color(Theme.PanelTextColorLight)
        height: labelContainer.model ? labelContainer.model.defaultRowHeight : 0
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    Column {
        id: labelsArea
        property QtObject parentModel: labelContainer.model
        anchors.top: txt.bottom
        visible: labelContainer.expanded
        Repeater {
            model: labelContainer.expanded ? labelContainer.labels.length : 0
            Loader {
                id: loader

                // Initially y == 0 for all the items. Don't enable them until they have been moved
                // into place.
                property int offset: (index === 0 || y > 0) ? (y + txt.height) : contentHeight
                active: labelContainer.contentBottom > offset
                width: labelContainer.width
                height: labelsArea.parentModel ? labelsArea.parentModel.rowHeight(index + 1) : 0

                sourceComponent: RowLabel {
                    label: labelContainer.labels[index];
                    onSelectBySelectionId: {
                        if (labelContainer.model.hasMixedTypesInExpandedState)
                            return;
                        if (labelContainer.reverseSelect) {
                            labelContainer.selectPrevBySelectionId(label.id);
                        } else {
                            labelContainer.selectNextBySelectionId(label.id);
                        }
                    }
                    onSetRowHeight: (newHeight) => {
                        labelsArea.parentModel.setExpandedRowHeight(index + 1, newHeight);
                        loader.height = labelsArea.parentModel.rowHeight(index + 1);
                    }
                }
            }
        }
    }

    ImageToolButton {
        id: notesButton
        anchors.verticalCenter: txt.verticalCenter
        anchors.right: expandButton.left
        implicitHeight: txt.height - 1
        property var eventIds: []
        property var texts: []
        property int currentNote: -1
        Connections {
            target: labelContainer.notesModel
            function onChanged(typeId, modelId, timelineIndex) {
                // This will only be called if notesModel != null.
                if (modelId === -1 || modelId === labelContainer.model.modelId) {
                    var notes =
                            labelContainer.notesModel.byTimelineModel(labelContainer.model.modelId);
                    var newTexts = [];
                    var newEventIds = [];
                    for (var i in notes) {
                        newTexts.push(notesModel.text(notes[i]))
                        newEventIds.push(notesModel.timelineIndex(notes[i]));
                    }

                    // Bindings are only triggered when assigning the whole array.
                    notesButton.eventIds = newEventIds;
                    notesButton.texts = newTexts;
                }
            }
        }

        visible: eventIds.length > 0
        imageSource: "image://icons/note"
        ToolTip.text: texts.join("\n");
        onClicked: {
            if (++currentNote >= eventIds.length)
                currentNote = 0;
            labelContainer.selectById(eventIds[currentNote]);
        }
    }

    ImageToolButton {
        id: expandButton
        anchors.verticalCenter: txt.verticalCenter
        anchors.right: parent.right
        implicitHeight: txt.height - 1
        enabled: labelContainer.expanded || (labelContainer.model && !labelContainer.model.empty)
        imageSource: labelContainer.expanded ? "image://icons/close_split" : "image://icons/split"
        ToolTip.text: labelContainer.expanded ? qsTranslate("QtC::Tracing", "Collapse category")
                                              : qsTranslate("QtC::Tracing", "Expand category")
        onClicked: labelContainer.model.expanded = !labelContainer.expanded
    }

    Rectangle {
        id: dragger
        property int visualIndex: labelContainer.visualIndex
        width: labelContainer.width
        height: labelContainer.height
        color: Theme.color(Theme.PanelStatusBarBackgroundColor)
        opacity: 0.5
        visible: Drag.active
        anchors.left: parent.left

        // anchor to top so that it reliably snaps back after dragging
        anchors.top: parent.top

        Drag.active: dragArea.drag.active
        Drag.onActiveChanged: {
            // We don't want text to be changed when reordering occurs, so we don't make
            // it a property
            draggerText.text = txt.text;
            if (Drag.active)
                labelContainer.dragStarted();
            else
                labelContainer.dragStopped();
        }

        states: [
            State {
                when: dragger.Drag.active
                ParentChange {
                    target: dragger
                    parent: labelContainer.draggerParent
                }
                PropertyChanges {
                    target: dragger
                    anchors.top: undefined
                }
            }
        ]

        TimelineText {
            id: draggerText
            visible: parent.Drag.active
            x: txt.x
            color: Theme.color(Theme.PanelTextColorLight)
            width: txt.width
            height: txt.height
            verticalAlignment: txt.verticalAlignment
        }
    }

    MouseArea {
        anchors.top: dragArea.bottom
        anchors.bottom: labelContainer.isDragging ? labelContainer.bottom : dragArea.bottom
        anchors.left: labelContainer.left
        anchors.right: labelContainer.right
        cursorShape: dragArea.cursorShape
    }

}
