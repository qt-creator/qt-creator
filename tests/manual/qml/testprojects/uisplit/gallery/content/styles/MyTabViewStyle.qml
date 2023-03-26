// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.2
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.1

Component {
TabViewStyle {
    tabOverlap: 16
    frameOverlap: 4
    tabsMovable: true

    frame: Rectangle {
        gradient: Gradient{
            GradientStop { color: "#e5e5e5" ; position: 0 }
            GradientStop { color: "#e0e0e0" ; position: 1 }
        }
        border.color: "#898989"
        Rectangle { anchors.fill: parent ; anchors.margins: 1 ; border.color: "white" ; color: "transparent" }
    }
    tab: Item {
        property int totalOverlap: tabOverlap * (control.count - 1)
        implicitWidth: Math.min ((styleData.availableWidth + totalOverlap)/control.count - 4, image.sourceSize.width)
        implicitHeight: image.sourceSize.height
        BorderImage {
            id: image
            anchors.fill: parent
            source: styleData.selected ? "../../images/tab_selected.png" : "../../images/tab.png"
            border.left: 30
            smooth: false
            border.right: 30
        }
        Text {
            text: styleData.title
            anchors.centerIn: parent
        }
    }
    leftCorner: Item { implicitWidth: 12 }
}
}
