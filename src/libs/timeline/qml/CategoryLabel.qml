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
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2

Item {
    id: labelContainer

    property QtObject model
    property QtObject notesModel
    property string text: model ? model.displayName : ""
    property bool expanded: model && model.expanded
    property var labels: model ? model.labels : []

    property bool dragging
    property int visualIndex
    property int dragOffset
    property Item draggerParent

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
        cursorShape: dragging ? Qt.ClosedHandCursor : Qt.OpenHandCursor
        drag.minimumY: dragging ? 0 : -dragOffset // Account for parent change below
        drag.maximumY: draggerParent.height - (dragging ? 0 : dragOffset)
    }

    DropArea {
        id: dropArea

        onPositionChanged: {
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

    Text {
        id: txt
        x: 5
        font.pixelSize: 12
        text: labelContainer.text
        color: "#232323"
        height: model ? model.defaultRowHeight : 0
        width: 140
        verticalAlignment: Text.AlignVCenter
        renderType: Text.NativeRendering
        elide: Text.ElideRight
    }

    Column {
        id: column
        property QtObject parentModel: model
        anchors.top: txt.bottom
        visible: expanded
        Repeater {
            model: labels.length
            SynchronousReloader {
                id: loader
                asynchronous: dragOffset - draggerParent.contentY + y + txt.height >
                              draggerParent.height

                active: expanded
                width: labelContainer.width
                height: column.parentModel ? column.parentModel.rowHeight(index + 1) : 0

                sourceComponent: RowLabel {
                    label: labels[index];
                    onSelectBySelectionId: {
                        if (labelContainer.reverseSelect) {
                            labelContainer.selectPrevBySelectionId(label.id);
                        } else {
                            labelContainer.selectNextBySelectionId(label.id);
                        }
                    }
                    onSetRowHeight: {
                        column.parentModel.setExpandedRowHeight(index + 1, newHeight);
                        loader.height = column.parentModel.rowHeight(index + 1);
                    }
                }
            }
        }
    }

    ToolButton {
        id: notesButton
        anchors.verticalCenter: txt.verticalCenter
        anchors.right: expandButton.left
        implicitWidth: 17
        implicitHeight: txt.height - 1
        property var eventIds: []
        property var texts: []
        property int currentNote: -1
        Connections {
            target: notesModel
            onChanged: {
                // This will only be called if notesModel != null.
                if (arguments[1] === -1 || arguments[1] === model.modelId) {
                    var notes = notesModel.byTimelineModel(model.modelId);
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
        iconSource: "ico_note.png"
        tooltip: texts.join("\n");
        onClicked: {
            if (++currentNote >= eventIds.length)
                currentNote = 0;
            labelContainer.selectById(eventIds[currentNote]);
        }
    }

    ToolButton {
        id: expandButton
        anchors.verticalCenter: txt.verticalCenter
        anchors.right: parent.right
        implicitWidth: 17
        implicitHeight: txt.height - 1
        enabled: expanded || (model && !model.empty)
        iconSource: expanded ? "arrow_down.png" : "arrow_right.png"
        tooltip: expanded ? qsTr("Collapse category") : qsTr("Expand category")
        onClicked: model.expanded = !expanded
    }

    Rectangle {
        id: dragger
        property int visualIndex: labelContainer.visualIndex
        width: labelContainer.width
        height: 0
        color: "black"
        opacity: 0.5
        anchors.left: parent.left

        // anchor to top so that it reliably snaps back after dragging
        anchors.top: parent.top

        Drag.active: dragArea.drag.active
        Drag.onActiveChanged: {
            // We don't want height or text to be changed when reordering occurs, so we don't make
            // them properties.
            draggerText.text = txt.text;
            if (Drag.active) {
                height = labelContainer.height;
                labelContainer.dragStarted();
            } else {
                height = 0;
                labelContainer.dragStopped();
            }
        }

        states: [
            State {
                when: dragger.Drag.active
                ParentChange {
                    target: dragger
                    parent: draggerParent
                }
                PropertyChanges {
                    target: dragger
                    anchors.top: undefined
                }
            }
        ]

        Text {
            id: draggerText
            visible: parent.Drag.active
            x: txt.x
            font.pixelSize: txt.font.pixelSize
            color: "white"
            width: txt.width
            height: txt.height
            verticalAlignment: txt.verticalAlignment
            renderType: txt.renderType
        }
    }

    MouseArea {
        anchors.top: dragArea.bottom
        anchors.bottom: labelContainer.dragging ? labelContainer.bottom : dragArea.bottom
        anchors.left: labelContainer.left
        anchors.right: labelContainer.right
        cursorShape: dragArea.cursorShape
    }

}
