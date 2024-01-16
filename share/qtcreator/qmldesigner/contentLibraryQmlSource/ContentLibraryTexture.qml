// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import HelperWidgets
import StudioTheme as StudioTheme
import ContentLibraryBackend
import WebFetcher

Item {
    id: root

    // Download states: "" (ie default, not downloaded), "unavailable", "downloading", "downloaded",
    //                  "failed"
    property string downloadState: modelData.isDownloaded() ? "downloaded" : ""
    property bool delegateVisible: modelData.textureVisible

    property bool _isUpdating: false

    property alias allowCancel: progressBar.closeButtonVisible
    property alias progressValue: progressBar.value
    property alias progressText: progressLabel.text

    visible: root.delegateVisible

    signal showContextMenu()

    function statusText() {
        if (root.downloadState === "downloaded")
            return qsTr("Texture was already downloaded.")
        if (root.downloadState === "unavailable")
            return qsTr("Network/Texture unavailable or broken Link.")
        if (root.downloadState === "failed")
            return qsTr("Could not download texture.")

        return qsTr("Click to download the texture.")
    }

    function startDownload(message) {
        if (root.downloadState !== "" && root.downloadState !== "failed")
            return

        root._startDownload(textureDownloader, message)
    }

    function updateTexture() {
        if (root.downloadState !== "downloaded")
            return

        root._isUpdating = true
        root._startDownload(textureDownloader, qsTr("Updating..."))
    }

    function _startDownload(downloader, message) {
        progressBar.visible = true
        tooltip.visible = false
        root.progressText = message
        root.allowCancel = true
        root.progressValue = Qt.binding(function() { return downloader.progress })

        root.downloadState = ""
        downloader.start()
    }

    MouseArea {
        id: mouseArea

        anchors.fill: parent
        hoverEnabled: !downloadIcon.visible
        acceptedButtons: Qt.LeftButton | Qt.RightButton

        onEntered: tooltip.visible = image.visible
        onExited: tooltip.visible = false

        onPressed: (mouse) => {
            if (mouse.button === Qt.LeftButton) {
                if (root.downloadState === "downloaded")
                    ContentLibraryBackend.rootView.startDragTexture(modelData, mapToGlobal(mouse.x, mouse.y))
            } else if (mouse.button === Qt.RightButton && root.downloadState === "downloaded") {
                root.showContextMenu()
            }
        }
    }

    Rectangle {
        id: downloadPane
        anchors.fill: parent
        color: StudioTheme.Values.themeThumbnailBackground
        border.color: "#00000000"

        visible: root.downloadState === "downloading"

        TextureProgressBar {
           id: progressBar
           anchors.rightMargin: 10
           anchors.leftMargin: 10

           anchors.left: parent.left
           anchors.right: parent.right
           anchors.verticalCenter: parent.verticalCenter

           visible: false

           onCancelRequested: {
               textureDownloader.cancel()
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
       }
    }

    Image {
        id: image
        anchors.fill: parent

        source: modelData.textureIcon
        visible: root.delegateVisible && root.downloadState != "downloading"
        cache: false

        property string webUrl: modelData.textureWebUrl

        IconButton {
            id: downloadIcon
            icon: root.downloadState === "unavailable"
                  ? StudioTheme.Constants.downloadUnavailable
                  : StudioTheme.Constants.download

            iconColor: root.downloadState === "unavailable" || root.downloadState === "failed"
                       ? StudioTheme.Values.themeRedLight
                       : "white"

            iconSize: 22
            iconScale: downloadIcon.containsMouse ? 1.2 : 1
            iconStyle: Text.Outline
            iconStyleColor: "black"

            tooltip: modelData.textureToolTip + (downloadIcon.visible
                                                ? "\n\n" + root.statusText()
                                                : "")
            buttonSize: 22

            transparentBg: true
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            visible: root.downloadState !== "downloaded"

            anchors.bottomMargin: 0
            anchors.rightMargin: 4

            Rectangle { // Arrow Fill
                anchors.centerIn: parent
                z: -1

                width: parent.width / 2
                height: parent.height / 2
                color: "black"
            }

            onClicked: {
                root.startDownload(qsTr("Downloading..."))
            }
        }

        IconButton {
            id: updateButton
            icon: StudioTheme.Constants.updateAvailable_medium
            iconColor: "white"
            tooltip: qsTr("Update texture")
            buttonSize: 22
            iconSize: 22

            iconScale: updateButton.containsMouse ? 1.2 : 1
            iconStyle: Text.Outline
            iconStyleColor: "black"

            anchors.left: parent.left
            anchors.bottom: parent.bottom

            visible: root.downloadState === "downloaded" && modelData.textureHasUpdate
            transparentBg: true

            onClicked: root.updateTexture()

            Text {
                text: StudioTheme.Constants.updateContent_medium
                font.family: StudioTheme.Constants.iconFont.family
                color: "black"

                anchors.horizontalCenter: parent.horizontalCenter
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 5

                font.pixelSize: 10
                font.bold: true

                scale: updateButton.containsMouse ? 1.2 : 1
            }
        }

        Rectangle {
            id: isNewFlag

            width: 32
            height: 32

            visible: downloadIcon.visible && modelData.textureIsNew
            color: "transparent"

            anchors.top: parent.top
            anchors.right: parent.right

            Image {
                source: "image://contentlibrary/new_flag_triangle.png"
                width: 32
                height: 32
            }

            Text {
                color: "white"
                font.family: StudioTheme.Constants.iconFont.family
                text: StudioTheme.Constants.favorite
                font.pixelSize: StudioTheme.Values.baseIconFontSize
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.topMargin: 2
                anchors.rightMargin: 4
            } // New texture flag
        }

        ToolTip {
            id: tooltip
            // contentWidth is not calculated correctly by the toolTip (resulting in a wider tooltip than
            // needed). Using a helper Text to calculate the correct width
            contentWidth: helperText.width
            bottomInset: -2
            text: modelData.textureToolTip + (downloadIcon.visible
                                              ? "\n\n" + root.statusText()
                                              : "")
            delay: 1000

            Text {
                id: helperText
                text: tooltip.text
                visible: false
            }
        }
    }

    FileDownloader {
        id: textureDownloader
        url: image.webUrl
        probeUrl: false
        downloadEnabled: true
        onDownloadStarting: {
            root.downloadState = "downloading"
        }

        onFinishedChanged: {
            root.progressText = qsTr("Extracting...")
            root.allowCancel = false
            root.progressValue = Qt.binding(function() { return textureExtractor.progress })

            textureExtractor.extract()
        }

        onDownloadCanceled: {
            root.progressText = ""
            root.progressValue = 0

            root.downloadState = ""
        }

        onDownloadFailed: {
            root.downloadState = "failed"
        }
    }

    Timer {
        id: delayedFinish
        interval: 200

        onTriggered: {
            modelData.setDownloaded()
            root.downloadState = modelData.isDownloaded() ? "downloaded" : "failed"
        }
    }

    FileExtractor {
        id: textureExtractor
        archiveName: textureDownloader.completeBaseName
        sourceFile: textureDownloader.outputFile
        targetPath: modelData.textureParentPath
        alwaysCreateDir: false
        clearTargetPathContents: false
        onFinishedChanged: {
            if (root._isUpdating)
                root._startDownload(iconDownloader, qsTr("Updating..."))
            else
                delayedFinish.restart()
        }
    }

    FileDownloader {
        id: iconDownloader
        url: modelData.textureWebIconUrl
        probeUrl: false
        downloadEnabled: true
        targetFilePath: modelData.textureIconPath
        overwriteTarget: true

        onDownloadStarting: {
            root.downloadState = "downloading"
        }

        onFinishedChanged: {
            image.source = ""
            image.source = modelData.textureIcon

            ContentLibraryBackend.rootView.markTextureUpdated(modelData.textureKey)

            delayedFinish.restart()
        }

        onDownloadCanceled: {
            root.progressText = ""
            root.progressValue = 0

            root.downloadState = ""
        }

        onDownloadFailed: {
            root.downloadState = "failed"
        }
    }
}
