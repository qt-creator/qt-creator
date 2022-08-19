// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick 2.1

MouseArea {
    property string direction

    anchors {
        top: direction === "up" ? parent.top : undefined
        bottom: direction === "down" ? parent.bottom : undefined
    }

    hoverEnabled: visible
    height: scrollerLoader.item.height
    width: parent.width

    Loader {
        id: scrollerLoader

        sourceComponent: menuItemDelegate
        property int index: -1
        property var modelData: {
            "visible": true,
            "scrollerDirection": direction,
            "enabled": true
        }
    }

    Timer {
        interval: 100
        repeat: true
        triggeredOnStart: true
        running: parent.containsMouse
        onTriggered: scrollABit()
    }
}
