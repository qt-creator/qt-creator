// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuickDesignerTheme 1.0
import HelperWidgets 2.0
import QtQuick.Controls

import StudioTheme 1.0 as StudioTheme

import WebFetcher 1.0
import ContentLibraryBackend

Item {
    id: root

    // Download states: "" (ie default, not downloaded), "unavailable", "downloading", "downloaded",
    //                  "failed"
    property string downloadState: modelData.isDownloaded() ? "downloaded" : ""
    property bool delegateVisible: modelData.textureVisible

    property alias allowCancel: progressBar.closeButtonVisible
    property alias progressValue: progressBar.value
    property alias progressText: progressLabel.text

    visible: root.delegateVisible

    signal showContextMenu()

    function statusText()
    {
        if (root.downloadState === "downloaded")
            return qsTr("Texture was already downloaded.")
        if (root.downloadState === "unavailable")
            return qsTr("Network/Texture unavailable or broken Link.")
        if (root.downloadState === "failed")
            return qsTr("Could not download texture.")

        return qsTr("Click to download the texture.")
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
               downloader.cancel()
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
                       : StudioTheme.Values.themeTextColor

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
                if (root.downloadState !== "" && root.downloadState !== "failed")
                    return

                progressBar.visible = true
                tooltip.visible = false
                root.progressText = qsTr("Downloading...")
                root.allowCancel = true
                root.progressValue = Qt.binding(function() { return downloader.progress })

                root.downloadState = ""
                downloader.start()
            }
        } // IconButton

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
    } // Image

    MouseArea {
        id: mouseArea

        anchors.fill: parent
        hoverEnabled: !downloadIcon.visible
        propagateComposedEvents: downloadIcon.visible
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

    FileDownloader {
        id: downloader
        url: image.webUrl
        probeUrl: false
        downloadEnabled: true
        onDownloadStarting: {
            root.downloadState = "downloading"
        }

        onFinishedChanged: {
            root.progressText = qsTr("Extracting...")
            root.allowCancel = false
            root.progressValue = Qt.binding(function() { return extractor.progress })

            extractor.extract()
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
        id: extractor
        archiveName: downloader.completeBaseName
        sourceFile: downloader.outputFile
        targetPath: modelData.textureParentPath
        alwaysCreateDir: false
        clearTargetPathContents: false
        onFinishedChanged: {
            delayedFinish.restart()
        }
    }
}
