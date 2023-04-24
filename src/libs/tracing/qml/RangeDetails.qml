// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls

import QtCreator.Tracing

Item {
    id: rangeDetails

    property real titleBarHeight: Theme.toolBarHeight() / 1.2
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
    signal updateNote(string text)

    visible: dialogTitle.length > 0

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
        height: rangeDetails.titleBarHeight
        color: Theme.color(Theme.Timeline_PanelHeaderColor)
        border.width: rangeDetails.borderWidth
        border.color: Theme.color(Theme.PanelTextColorMid)

        TimelineText {
            id: typeTitle
            text: rangeDetails.dialogTitle
            font.bold: true
            verticalAlignment: Text.AlignVCenter
            anchors.left: parent.left
            anchors.right: closeIcon.left
            anchors.leftMargin: rangeDetails.outerMargin
            anchors.rightMargin: rangeDetails.innerMargin
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
            ToolTip.text: qsTranslate("QtC::Tracing", "Edit note")
        }

        ImageToolButton {
            id: lockIcon
            imageSource: "image://icons/lock_" + (rangeDetails.locked ? "closed" : "open")
            anchors.top: closeIcon.top
            anchors.right: closeIcon.left
            implicitHeight: typeTitle.height
            onClicked: rangeDetails.locked = !rangeDetails.locked
            ToolTip.text: qsTranslate("QtC::Tracing", "View event information on mouseover.")
        }

        ImageToolButton {
            id: closeIcon
            anchors.right: parent.right
            anchors.top: parent.top
            implicitHeight: typeTitle.height
            imageSource: "image://icons/arrow" + (col.visible ? "up" : "down")
            onClicked: col.visible = !col.visible
            ToolTip.text: col.visible ? qsTranslate("QtC::Tracing", "Collapse")
                                      : qsTranslate("QtC::Tracing", "Expand")
        }
    }

    Rectangle {
        id: contentArea
        color: Theme.color(Theme.Timeline_PanelBackgroundColor)

        anchors.top: titleBar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: dragHandle.bottom

        border.width: rangeDetails.borderWidth
        border.color: Theme.color(Theme.PanelTextColorMid)
    }

    Grid {
        id: col

        anchors.left: parent.left
        anchors.top: titleBar.bottom
        anchors.topMargin: rangeDetails.innerMargin
        anchors.leftMargin: rangeDetails.outerMargin
        anchors.rightMargin: rangeDetails.outerMargin

        height: visible ? implicitHeight : 0

        spacing: rangeDetails.innerMargin
        columns: 2

        property int minimumWidth: rangeDetails.minimumInnerWidth
        onPositioningComplete: {
            // max(width of longest label * 2, minimumInnerWidth)
            var result = rangeDetails.minimumInnerWidth;
            for (var i = 0; i < children.length; ++i) {
                if (children[i].isLabel)
                    result = Math.max(children[i].implicitWidth * 2 + innerMargin, result);
            }

            minimumWidth = result + 2 * outerMargin;
        }

        property int labelWidth: Math.ceil((minimumWidth - rangeDetails.innerMargin) / 2)
                                           - rangeDetails.outerMargin
        property int valueWidth: dragHandle.x - labelWidth - rangeDetails.innerMargin
                                 - rangeDetails.outerMargin

        onMinimumWidthChanged: {
            if (dragHandle.x < minimumWidth - rangeDetails.outerMargin)
                dragHandle.x = minimumWidth - rangeDetails.outerMargin;
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
        anchors.leftMargin: rangeDetails.outerMargin
        anchors.rightMargin: rangeDetails.outerMargin
        anchors.topMargin: visible ? rangeDetails.innerMargin : 0
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
                if (!noteEdit.readOnly)
                    rangeDetails.updateNote(noteEdit.text);
            }
            interval: 1000
        }

    }

    Item {
        id: dragHandle
        width: rangeDetails.outerMargin
        height: rangeDetails.outerMargin
        x: rangeDetails.initialWidth
        anchors.top: noteEdit.bottom
        clip: true
        MouseArea {
            anchors.fill: parent
            drag.target: parent
            drag.minimumX: col.minimumWidth - rangeDetails.outerMargin
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
