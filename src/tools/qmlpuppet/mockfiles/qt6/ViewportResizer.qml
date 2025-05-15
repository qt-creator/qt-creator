// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick

// Custom resizer for viewports, dynamically applied based on current preset
Item {
    id: root

    property real containerSize
    property int orientation
    readonly property alias containsMouse: mouseArea.containsMouse
    readonly property alias dragActive: mouseArea.drag.active

    property real _dragMin: containerSize * 0.1
    property real _dragMax: containerSize * 0.9
    readonly property real grabSize: 4

    width: orientation === Qt.Vertical ? grabSize * 2 : parent.width
    height: orientation === Qt.Horizontal ? grabSize * 2 : parent.height

    signal currentDividerChanged(real value)

    MouseArea {
        id: mouseArea

        anchors.fill: parent
        drag.target: root
        drag.axis: orientation === Qt.Vertical ? Drag.XAxis : Drag.YAxis
        drag.minimumX: _dragMin
        drag.maximumX: _dragMax
        drag.minimumY: _dragMin
        drag.maximumY: _dragMax
        drag.smoothed: true
        hoverEnabled: true

        onPositionChanged: {
            var deltaPx = (orientation === Qt.Vertical ? root.x : root.y) + grabSize;
            var deltaNorm = (deltaPx / root.containerSize);
            currentDividerChanged(deltaNorm);
        }
    }
}
