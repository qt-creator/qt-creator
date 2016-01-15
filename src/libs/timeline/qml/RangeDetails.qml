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

Item {
    id: rangeDetails

    property string duration
    property string label
    property string dialogTitle
    property string file
    property int line
    property int column
    property bool isBindingLoop
    property bool hasContents

    property int selectedModel: -1
    property int selectedItem: -1

    property bool locked

    property var models
    property var notes

    signal recenterOnItem
    signal toggleSelectionLocked
    signal clearSelection

    width: col.width + 25
    height: hasContents ? contentArea.height + titleBar.height : 0

    function hide() {
        noteEdit.focus = false;
        hasContents = false;
        selectedModel = selectedItem = -1;
        noteEdit.text = "";
        duration = "";
        label = "";
        file = "";
        line = -1;
        column = 0;
        isBindingLoop = false;
    }

    Connections {
        target: rangeDetails.parent
        // keep inside view
        onWidthChanged: fitInView();
        onHeightChanged: fitInView();
    }

    ListModel {
        id: eventInfo
    }

    function showInfo(model, item) {
        // make sure we don't accidentally save the old text for the new event
        noteEdit.focus = false;

        selectedModel = model;
        selectedItem = item;
        var timelineModel = models[selectedModel];
        var eventData = timelineModel.details(selectedItem)
        eventInfo.clear();
        for (var k in eventData) {
            if (k === "displayName") {
                dialogTitle = eventData[k];
            } else if (eventData[k].length > 0) {
                eventInfo.append({content : k});
                eventInfo.append({content : eventData[k]});
            }
        }
        hasContents = eventInfo.count > 0;

        var location = timelineModel.location(selectedItem)
        if (location.hasOwnProperty("file")) { // not empty
            file = location.file;
            line = location.line;
            column = location.column;
        } else {
            // reset to default values
            file = "";
            line = 0;
            column = -1;
        }

        noteEdit.focus = false;
        var noteId = notes ? notes.get(timelineModel.modelId, selectedItem) : -1;
        noteEdit.text = (noteId !== -1) ? notes.text(noteId) : "";
    }

    function fitInView() {
        // don't reposition if it does not fit
        if (parent.width < width || parent.height < height)
            return;

        if (x + width > parent.width)
            x = parent.width - width;
        if (x < 0)
            x = 0;
        if (y + height > parent.height)
            y = parent.height - height;
        if (y < 0)
            y = 0;
    }

    // shadow
    BorderImage {
        property int px: 4
        source: "dialog_shadow.png"

        border {
            left: px; top: px
            right: px; bottom: px
        }
        width: parent.width + 2*px - 1
        height: parent.height
        x: -px + 1
        y: px + 1
    }

    Rectangle {
        id: titleBar
        width: parent.width
        height: 20
        color: "#55a3b8"
        radius: 5
        border.width: 1
        border.color: "#a0a0a0"
    }
    Item {
        width: parent.width+1
        height: 11
        y: 10
        clip: true
        Rectangle {
            width: parent.width-1
            height: 15
            y: -5
            color: "#55a3b8"
            border.width: 1
            border.color: "#a0a0a0"
        }
    }

    //title
    TimelineText {
        id: typeTitle
        text: "  "+rangeDetails.dialogTitle
        font.bold: true
        height: 18
        y: 2
        verticalAlignment: Text.AlignVCenter
        anchors.left: parent.left
        anchors.right: editIcon.left
        color: "white"
        elide: Text.ElideRight
    }

    // Details area
    Rectangle {
        id: contentArea
        color: "white"
        width: parent.width
        height: 10 + col.height + (noteEdit.visible ? (noteEdit.height + 5) : 0)
        y: 20
        border.width: 1
        border.color: "#a0a0a0"

        //details
        Grid {
            id: col
            x: 10
            y: 5
            spacing: 5
            columns: 2
            property int minimumWidth: {
                var result = 150;
                for (var i = 0; i < children.length; ++i)
                    result = Math.max(children[i].x, result);
                return result + 20;
            }

            onMinimumWidthChanged: {
                if (dragHandle.x < minimumWidth)
                    dragHandle.x = minimumWidth;
            }

            Repeater {
                model: eventInfo
                Detail {
                    valueWidth: dragHandle.x - x - 15
                    isLabel: index % 2 === 0
                    text: (content === undefined) ? "" : (isLabel ? (content + ":") : content)
                }
            }
        }


        TextEdit {
            id: noteEdit
            x: 10
            anchors.topMargin: 5
            anchors.bottomMargin: 5
            anchors.top: col.bottom

            visible: notes && (text.length > 0 || focus)
            width: col.width
            wrapMode: Text.Wrap
            color: "orange"
            font.italic: true
            font.pixelSize: typeTitle.font.pixelSize
            font.family: typeTitle.font.family
            renderType: typeTitle.renderType
            selectByMouse: true
            onTextChanged: saveTimer.restart()
            onFocusChanged: {
                if (!focus && selectedModel != -1 && selectedItem != -1) {
                    saveTimer.stop();
                    if (notes)
                        notes.setText(models[selectedModel].modelId, selectedItem, text);
                }
            }

            Timer {
                id: saveTimer
                onTriggered: {
                    if (notes && selectedModel != -1 && selectedItem != -1)
                        notes.setText(models[selectedModel].modelId, selectedItem, noteEdit.text);
                }
                interval: 1000
            }
        }
    }

    MouseArea {
        anchors.fill: parent
        drag.target: parent
        drag.minimumX: 0
        drag.maximumX: rangeDetails.parent.width - rangeDetails.width
        drag.minimumY: 0
        drag.maximumY: rangeDetails.parent.height - rangeDetails.height
        onClicked: rangeDetails.recenterOnItem()
    }

    Image {
        id: editIcon
        source: "ico_edit.png"
        anchors.top: closeIcon.top
        anchors.right: lockIcon.left
        anchors.rightMargin: 4
        visible: notes
        width: 8
        height: 12
        MouseArea {
            anchors.fill: parent
            onClicked: noteEdit.focus = true
        }
    }

    Image {
        id: lockIcon
        source: locked?"lock_closed.png" : "lock_open.png"
        anchors.top: closeIcon.top
        anchors.right: closeIcon.left
        anchors.rightMargin: 4
        width: 8
        height: 12
        MouseArea {
            anchors.fill: parent
            onClicked: rangeDetails.toggleSelectionLocked()
        }
    }


    TimelineText {
        id: closeIcon
        x: col.width + 10
        y: 4
        text:"X"
        color: "white"
        MouseArea {
            anchors.fill: parent
            onClicked: rangeDetails.clearSelection()
        }
    }

    Item {
        id: dragHandle
        width: 10
        height: 10
        x: 300
        anchors.bottom: parent.bottom
        clip: true
        MouseArea {
            anchors.fill: parent
            drag.target: parent
            drag.minimumX: col.minimumWidth
            drag.axis: Drag.XAxis
            cursorShape: Qt.SizeHorCursor
        }
        Rectangle {
            color: "#55a3b8"
            rotation: 45
            width: parent.width * Math.SQRT2
            height: parent.height * Math.SQRT2
            x: parent.width - width / 2
            y: parent.height - height / 2
        }
    }
}
