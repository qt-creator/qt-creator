// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

import QtQuick

import StudioTheme 1.0 as StudioTheme

Rectangle {
    id: root

    color: StudioTheme.Values.themeThumbnailBackground
    border.color: "#00000000"

    signal requestCancel

    property alias allowCancel: progressBar.closeButtonVisible
    property alias progressValue: progressBar.value
    property alias progressLabel: progressLabel.text

    function beginDownload(progressFunction)
    {
        progressBar.visible = true
        root.progressLabel = qsTr("Downloading...")
        root.allowCancel = true
        root.progressValue = progressFunction
    }

    function endDownload()
    {
        root.allowCancel = false
        root.progressLabel = ""
        root.progressValue = 0
    }

    TextureProgressBar {
       id: progressBar
       anchors.rightMargin: 10
       anchors.leftMargin: 10

       anchors.left: parent.left
       anchors.right: parent.right
       anchors.verticalCenter: parent.verticalCenter

       visible: false

       onCancelRequested: {
           root.requestCancel()
       }

       Text {
           id: progressLabel
           color: StudioTheme.Values.themeTextColor
           text: qsTr("Progress:")
           anchors.bottom: parent.top
           anchors.bottomMargin: 5
           anchors.left: parent.left
           font.pixelSize: 12
       }

       Row {
           anchors.top: parent.bottom
           anchors.topMargin: 5
           anchors.horizontalCenter: parent.horizontalCenter

           Text {
               id: progressAmount
               color: StudioTheme.Values.themeTextColor
               text: progressBar.value.toFixed(1)

               font.pixelSize: 12
           }

           Text {
               id: percentSign
               color: StudioTheme.Values.themeTextColor
               text: qsTr("%")
               font.pixelSize: 12
           }
       }
   } // TextureProgressBar
} // Rectangle
