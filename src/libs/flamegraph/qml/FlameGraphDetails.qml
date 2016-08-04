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
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4

Item {
    id: rangeDetails

    property color titleBarColor: "#55a3b8"
    property color titleBarTextColor: "white"
    property color contentColor: "white"
    property color contentTextColor: "black"
    property color borderColor: "#a0a0a0"
    property color noteTextColor: "orange"
    property color buttonSelectedColor: titleBarColor
    property color buttonHoveredColor: titleBarColor

    property real titleBarHeight: 20
    property real borderWidth: 1
    property real outerMargin: 10
    property real innerMargin: 5
    property real minimumInnerWidth: 150
    property real initialWidth: 300

    property real minimumX
    property real maximumX
    property real minimumY
    property real maximumY

    property string dialogTitle
    property var model
    property string note

    signal clearSelection

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
    }

    Rectangle {
        id: titleBar
        width: parent.width
        height: titleBarHeight
        color: titleBarColor
        border.width: borderWidth
        border.color: borderColor

        FlameGraphText {
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
            color: titleBarTextColor
            elide: Text.ElideRight
        }

        ToolButton {
            id: closeIcon

            implicitWidth: 30
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            onClicked: rangeDetails.clearSelection()

            Image {
                id: image
                source: "image://icons/close_window" + (parent.enabled ? "" : "/disabled")
                width: 16
                height: 16
                anchors.centerIn: parent
            }

            style: ButtonStyle {
                background: Rectangle {
                    color: (control.checked || control.pressed)
                           ? buttonSelectedColor
                           : control.hovered
                             ? buttonHoveredColor
                             : "#00000000"
                }
            }
        }
    }

    Rectangle {
        id: contentArea
        color: contentColor

        anchors.top: titleBar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: dragHandle.bottom

        border.width: borderWidth
        border.color: borderColor
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
        property int minimumWidth: {
            var result = minimumInnerWidth;
            for (var i = 0; i < children.length; ++i)
                result = Math.max(children[i].x, result);
            return result + 2 * outerMargin;
        }

        onMinimumWidthChanged: {
            if (dragHandle.x < minimumWidth)
                dragHandle.x = minimumWidth;
        }

        Repeater {
            model: rangeDetails.model
            FlameGraphText {
                property bool isLabel: index % 2 === 0
                font.bold: isLabel
                elide: Text.ElideRight
                width: text === "" ? 0 : (isLabel ? implicitWidth :
                                                    (dragHandle.x - x - innerMargin))
                text: isLabel ? (modelData + ":") : modelData
                color: contentTextColor
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

        readOnly: true
        visible: text.length > 0
        text: note
        wrapMode: Text.Wrap
        color: noteTextColor
        font.italic: true
        font.pixelSize: typeTitle.font.pixelSize
        font.family: typeTitle.font.family
        renderType: typeTitle.renderType
        selectByMouse: true
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
            drag.minimumX: col.minimumWidth
            drag.axis: Drag.XAxis
            cursorShape: Qt.SizeHorCursor
        }
        Rectangle {
            color: titleBarColor
            rotation: 45
            width: parent.width * Math.SQRT2
            height: parent.height * Math.SQRT2
            x: parent.width - width / 2
            y: parent.height - height / 2
        }
    }
}
