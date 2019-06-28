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

import QtQuick 2.9
import QtQuick.Controls 2.0

import TimelineTheme 1.0

Item {
    id: rangeDetails

    property real titleBarHeight: 20
    property real borderWidth: 1
    property real outerMargin: 10
    property real innerMargin: 5
    property real minimumInnerWidth: 150
    property real initialWidth: 300

    property real minimumX: 0
    property real maximumX: parent.width
    property real minimumY: 0
    property real maximumY: parent.height

    property string dialogTitle: ""
    property string file: ""
    property int line: -1
    property int column: -1

    property bool locked: false
    property var model: []

    property alias noteText: noteEdit.text
    property alias noteFocus: noteEdit.focus
    property alias noteReadonly: noteEdit.readOnly

    signal recenterOnItem
    signal clearSelection
    signal updateNote(string text)

    visible: dialogTitle.length > 0 || model.length > 0

    width: dragHandle.x + dragHandle.width
    height: contentArea.height + titleBar.height

    onMinimumXChanged: x = Math.max(x, minimumX)
    onMaximumXChanged: x = Math.min(x, Math.max(minimumX, maximumX - width))
    onMinimumYChanged: y = Math.max(y, minimumY)
    onMaximumYChanged: y = Math.min(y, Math.max(minimumY, maximumY - height))

    MouseArea {
        anchors.fill: parent
        drag.target: parent
        drag.minimumX: parent.minimumX
        drag.maximumX: parent.maximumX - rangeDetails.width
        drag.minimumY: parent.minimumY
        drag.maximumY: parent.maximumY - rangeDetails.height
        onClicked: rangeDetails.recenterOnItem()
    }

    Rectangle {
        id: titleBar
        width: parent.width
        height: titleBarHeight
        color: Theme.color(Theme.Timeline_PanelHeaderColor)
        border.width: borderWidth
        border.color: Theme.color(Theme.PanelTextColorMid)

        TimelineText {
            id: typeTitle
            text: rangeDetails.dialogTitle
            font.bold: true
            verticalAlignment: Text.AlignVCenter
            anchors.left: parent.left
            anchors.right: closeIcon.left
            anchors.leftMargin: outerMargin
            anchors.rightMargin: innerMargin
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            color: Theme.color(Theme.PanelTextColorLight)
            elide: Text.ElideRight
        }

        ImageToolButton {
            id: editIcon
            imageSource: "image://icons/edit"
            anchors.top: parent.top
            anchors.right: lockIcon.left
            implicitHeight: typeTitle.height
            visible: !rangeDetails.noteReadonly
            onClicked: noteEdit.focus = true
            ToolTip.text: qsTr("Edit note")
        }

        ImageToolButton {
            id: lockIcon
            imageSource: "image://icons/lock_" + (locked ? "closed" : "open")
            anchors.top: closeIcon.top
            anchors.right: closeIcon.left
            implicitHeight: typeTitle.height
            onClicked: locked = !locked
            ToolTip.text: qsTr("View event information on mouseover.")
        }

        ImageToolButton {
            id: closeIcon
            anchors.right: parent.right
            anchors.top: parent.top
            implicitHeight: typeTitle.height
            imageSource: "image://icons/close_window"
            onClicked: rangeDetails.clearSelection()
            ToolTip.text: qsTr("Close")
        }
    }

    Rectangle {
        id: contentArea
        color: Theme.color(Theme.Timeline_PanelBackgroundColor)

        anchors.top: titleBar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: dragHandle.bottom

        border.width: borderWidth
        border.color: Theme.color(Theme.PanelTextColorMid)
    }

    Grid {
        id: col

        anchors.left: parent.left
        anchors.top: titleBar.bottom
        anchors.topMargin: innerMargin
        anchors.leftMargin: outerMargin
        anchors.rightMargin: outerMargin

        spacing: innerMargin
        columns: 2

        property int minimumWidth: minimumInnerWidth
        onPositioningComplete: {
            // max(width of longest label * 2, minimumInnerWidth)
            var result = minimumInnerWidth;
            for (var i = 0; i < children.length; ++i) {
                if (children[i].isLabel)
                    result = Math.max(children[i].implicitWidth * 2 + innerMargin, result);
            }

            minimumWidth = result + 2 * outerMargin;
        }

        property int labelWidth: Math.ceil((minimumWidth - innerMargin) / 2) - outerMargin
        property int valueWidth: dragHandle.x - labelWidth - innerMargin - outerMargin

        onMinimumWidthChanged: {
            if (dragHandle.x < minimumWidth - outerMargin)
                dragHandle.x = minimumWidth - outerMargin;
        }

        Repeater {
            model: rangeDetails.model
            Detail {
                labelWidth: col.labelWidth
                valueWidth: col.valueWidth
                isLabel: index % 2 === 0
                text: isLabel ? (modelData + ":") : modelData
            }
        }
    }


    TextEdit {
        id: noteEdit

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: outerMargin
        anchors.rightMargin: outerMargin
        anchors.topMargin: visible ? innerMargin : 0
        anchors.top: col.bottom
        height: visible ? implicitHeight : 0

        readOnly: rangeDetails.noteReadonly
        visible: (text.length > 0 || focus)
        wrapMode: Text.Wrap
        color: Theme.color(Theme.Timeline_HighlightColor)
        font.italic: true
        font.pixelSize: typeTitle.font.pixelSize
        font.family: typeTitle.font.family
        renderType: typeTitle.renderType
        selectByMouse: true
        onTextChanged: saveTimer.restart()
        onFocusChanged: {
            if (!focus) {
                saveTimer.stop();
                if (!readOnly)
                    rangeDetails.updateNote(text);
            }
        }

        Timer {
            id: saveTimer
            onTriggered: {
                if (!rangeDetails.readOnly)
                    rangeDetails.updateNote(noteEdit.text);
            }
            interval: 1000
        }

    }

    Item {
        id: dragHandle
        width: outerMargin
        height: outerMargin
        x: initialWidth
        anchors.top: noteEdit.bottom
        clip: true
        MouseArea {
            anchors.fill: parent
            drag.target: parent
            drag.minimumX: col.minimumWidth - outerMargin
            drag.axis: Drag.XAxis
            cursorShape: Qt.SizeHorCursor
        }
        Rectangle {
            color: Theme.color(Theme.Timeline_PanelHeaderColor)
            rotation: 45
            width: parent.width * Math.SQRT2
            height: parent.height * Math.SQRT2
            x: parent.width - width / 2
            y: parent.height - height / 2
        }
    }
}
