// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick

// Custom resizer for viewports, dynamically applied based on current preset
Item {
    id: root

    property real divider
    property real containerSize
    property int orientation

    property real _dragMin: 0.0
    property real _dragMax: 0.0
    readonly property real _size: 12

    width: orientation === Qt.Vertical ? _size : parent.width
    height: orientation === Qt.Horizontal ? _size : parent.height

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

        onPositionChanged: {
            var deltaPx = (orientation === Qt.Vertical ? root.x : root.y);
            var deltaNorm = deltaPx / root.containerSize;
            currentDividerChanged(deltaNorm);
        }

        Component.onCompleted: {
            _dragMin = containerSize * 0.1; // Drag constraint
            _dragMax = containerSize - _dragMin;
        }
    }
}
