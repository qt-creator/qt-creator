// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import WelcomeScreen 1.0

Rectangle {
    id: root
    color: Constants.currentNormalThumbnailBackground

    property alias value: progressBar.value
    property alias text: progressLabel.text
    property alias allowCancel: progressBar.closeButtonVisible

    readonly property int pixelSize: 12
    readonly property int textMargin: 5

    signal cancelRequested

    DownloadProgressBar {
        id: progressBar
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: pushButton.top
        anchors.bottomMargin: 40
        anchors.rightMargin: 10
        anchors.leftMargin: 10

        onCancelRequested: root.cancelRequested()

        Text {
            id: progressLabel
            color: Constants.currentGlobalText
            text: qsTr("Progress:")
            anchors.top: parent.bottom
            anchors.topMargin: root.textMargin
            anchors.left: parent.left
            font.pixelSize: root.pixelSize
        }

        Text {
            id: progressAmount
            color: Constants.currentGlobalText
            text: stringMapper.text
            anchors.top: parent.bottom
            anchors.topMargin: root.textMargin
            anchors.right: percentSign.left
            anchors.rightMargin: root.textMargin
            font.pixelSize: root.pixelSize
        }

        Text {
            id: percentSign
            color: Constants.currentGlobalText
            text: qsTr("%")
            anchors.right: parent.right
            anchors.top: parent.bottom
            anchors.topMargin: root.textMargin
            font.pixelSize: root.pixelSize
        }
    }

    PushButton {
        id: pushButton
        y: 177
        visible: progressBar.downloadFinished
        text: qsTr("Open")
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottomMargin: 40
    }

    StringMapper {
        id: stringMapper
        decimals: 1
        input: root.value
    }
}
