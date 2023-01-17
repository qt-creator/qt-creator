// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls

import StudioTheme as StudioTheme

Item {
    id: root
    width: 272
    height: 25
    property int value: 0
    property bool closeButtonVisible

    readonly property int margin: 4

    readonly property string qdsBrand: "#57B9FC"

    signal cancelRequested

    Rectangle {
        id: progressBarGroove
        color: StudioTheme.Values.themeThumbnailLabelBackground
        anchors.fill: parent
    }

    Rectangle {
        id: progressBarTrack
        width: root.value * ((root.width - closeButton.width) - 2 * root.margin) / 100
        color: root.qdsBrand
        border.color: "#002e769e"
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.margins: root.margin
    }

    Text {
        id: closeButton
        visible: root.closeButtonVisible
        width: 20
        text: StudioTheme.Constants.closeCross
        color: root.qdsBrand
        horizontalAlignment: Qt.AlignHCenter
        verticalAlignment: Qt.AlignVCenter
        font.family: StudioTheme.Constants.iconFont.family
        font.pixelSize: StudioTheme.Values.myIconFontSize

        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.margins: root.margin

        MouseArea {
            anchors.fill: parent
            onClicked: {
                root.cancelRequested()
            }
        }
    }
}
