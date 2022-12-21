// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtCreator.Tracing

Item {
    id: flamegraphItem
    property color borderColor
    property real borderWidth
    property real itemHeight
    property bool isSelected
    property string text;

    signal mouseEntered
    signal mouseExited
    signal clicked
    signal doubleClicked

    property bool textVisible: width > 20 || isSelected
    property int level: parent.level !== undefined ? parent.level + 1 : 0

    height: parent === null ?
                0 : (parent.height - (parent.itemHeight !== undefined ? parent.itemHeight : 0));
    width: parent === null ? 0 : parent.width * FlameGraph.relativeSize
    x: parent === null ? 0 : parent.width * FlameGraph.relativePosition

    Rectangle {
        border.color: flamegraphItem.borderColor
        border.width: flamegraphItem.borderWidth
        color: Qt.hsla((flamegraphItem.level % 12) / 72, 0.9 + Math.random() / 10,
                       0.45 + Math.random() / 10, 0.9 + Math.random() / 10);
        height: flamegraphItem.itemHeight
        anchors.left: flamegraphItem.left
        anchors.right: flamegraphItem.right
        anchors.bottom: flamegraphItem.bottom

        TimelineText {
            id: text
            visible: flamegraphItem.textVisible
            anchors.fill: parent
            anchors.margins: 5
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignHCenter
            text: flamegraphItem.text
            elide: Text.ElideRight
            wrapMode: Text.WrapAtWordBoundaryOrAnywhere
            font.bold: flamegraphItem.isSelected
        }

        MouseArea {
            anchors.fill: parent
            hoverEnabled: true

            onEntered: flamegraphItem.mouseEntered()
            onExited: flamegraphItem.mouseExited()
            onClicked: flamegraphItem.clicked()
            onDoubleClicked: flamegraphItem.doubleClicked()
        }
    }
}
