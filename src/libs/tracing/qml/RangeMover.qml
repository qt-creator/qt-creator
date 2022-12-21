// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtCreator.Tracing

Item {
    id: rangeMover
    anchors.fill: parent
    signal rangeDoubleClicked()

    property color handleColor: Theme.color(Theme.Timeline_HandleColor)
    property color rangeColor: Theme.color(Theme.Timeline_RangeColor)
    property color dragColor: Theme.color(Theme.Timeline_RangeColor)
    property color borderColor: Theme.color(Theme.Timeline_RangeColor)
    property color singleLineColor: Theme.color(Theme.Timeline_RangeColor)

    property alias rangeLeft: leftRange.x
    property alias rangeRight: rightRange.x
    readonly property alias rangeWidth: selectedRange.width

    Rectangle {
        id: selectedRange

        x: leftRange.x
        width: rightRange.x - leftRange.x
        height: parent.height

        function alphaColor(color) {
            return Qt.rgba(color.r, color.g, color.b, Math.max(Math.min(color.a, 0.7), 0.3));
        }

        color: width > 1 ? alphaColor(dragArea.pressed ? rangeMover.dragColor
                                                       : rangeMover.rangeColor)
                         : rangeMover.singleLineColor
    }

    Item {
        id: leftRange

        onXChanged: {
            if (dragArea.drag.active)
                rightRange.x = x + dragArea.origWidth;
        }

        x: 0
        height: parent.height

        Rectangle {
            id: leftBorderHandle
            height: parent.height
            anchors.right: parent.left
            width: 7
            color: rangeMover.handleColor
            visible: false
            Image {
                source: "image://icons/range_handle"
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.margins: 1
                y: (parent.height - height) / 2
            }
        }

        states: State {
            name: "highlighted"
            PropertyChanges {
                target: leftBorderHandle
                visible: true
            }
        }

        MouseArea {
            anchors.fill: leftBorderHandle

            drag.target: leftRange
            drag.axis: Drag.XAxis
            drag.minimumX: 0
            drag.maximumX: rangeMover.width
            drag.onActiveChanged: drag.maximumX = rightRange.x

            hoverEnabled: true

            onEntered: {
                parent.state = "highlighted";
            }
            onExited: {
                if (!pressed) parent.state = "";
            }
            onReleased: {
                if (!containsMouse) parent.state = "";
            }
        }
    }

    Item {
        id: rightRange

        x: 1
        height: parent.height

        Rectangle {
            id: rightBorderHandle
            height: parent.height
            anchors.left: parent.right
            width: 7
            color: rangeMover.handleColor
            visible: false
            Image {
                source: "image://icons/range_handle"
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.margins: 1
                y: (parent.height - height) / 2
            }
        }

        states: State {
            name: "highlighted"
            PropertyChanges {
                target: rightBorderHandle
                visible: true
            }
        }

        MouseArea {
            anchors.fill: rightBorderHandle

            drag.target: rightRange
            drag.axis: Drag.XAxis
            drag.minimumX: 0
            drag.maximumX: rangeMover.width
            drag.onActiveChanged: drag.minimumX = leftRange.x

            hoverEnabled: true

            onEntered: {
                parent.state = "highlighted";
            }
            onExited: {
                if (!pressed) parent.state = "";
            }
            onReleased: {
                if (!containsMouse) parent.state = "";
            }
        }
    }

    MouseArea {
        id: dragArea
        property double origWidth: 0

        anchors.fill: selectedRange
        drag.target: leftRange
        drag.axis: Drag.XAxis
        drag.minimumX: 0
        drag.maximumX: rangeMover.width - origWidth
        drag.onActiveChanged: origWidth = selectedRange.width
        onDoubleClicked: parent.rangeDoubleClicked()
    }
}
