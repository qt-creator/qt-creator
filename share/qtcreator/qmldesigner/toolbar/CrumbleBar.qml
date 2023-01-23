// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls

ListView {
    id: root

    property real crumbleWidth: 166
    property real crumbleHeight: 36

    property real inset: 5
    property real strokeWidth: 1
    property real textLeftMargin: 18
    property real textTopMargin: 6
    property real textRightMargin: 6
    property real textBottomMargin: 6

    property alias font: fontMetrics.font

    signal clicked(index: int)

    boundsBehavior: Flickable.StopAtBounds
    flickableDirection: Flickable.HorizontalFlick
    interactive: true

    orientation: ListView.Horizontal
    clip: true
    focus: true

    function updateContentX() {
         root.contentX = (root.contentWidth < root.width) ? 0 : root.contentWidth - root.width
    }

    onCountChanged: root.updateContentX()
    onWidthChanged: root.updateContentX()

    visible: root.count > 1 && root.width > (root.crumbleWidth) - 24

    delegate: CrumbleBread {
        text: fileName
        tooltip: fileAddress

        width: root.crumbleWidth
        height: root.crumbleHeight

        inset: root.inset
        strokeWidth: root.strokeWidth
        textLeftMargin: root.textLeftMargin
        textTopMargin: root.textTopMargin
        textRightMargin: root.textRightMargin
        textBottomMargin: root.textBottomMargin

        font: root.font

        modelSize: root.count

        onClicked: {
            if (index + 1 < root.count)
                root.clicked(index)
        }
    }

    add: Transition {
        NumberAnimation {
            property: "opacity"
            from: 0
            to: 1.0
            duration: 400
        }
        NumberAnimation {
            property: "scale"
            from: 0
            to: 1.0
            duration: 400
        }
    }

    displaced: Transition {
        NumberAnimation {
            property: "x"
            duration: 400
            easing.type: Easing.OutBack
        }
    }

    FontMetrics {
        id: fontMetrics
        font.pixelSize: 12
        font.family: "SF Pro"
    }
}
