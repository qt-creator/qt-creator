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

    signal showContextMenu()

    // Download states: "" (ie default, not downloaded), "unavailable", "downloading", "downloaded",
    //                  "failed"
    property string downloadState: modelData.isDownloaded() ? "downloaded" : ""

    visible: modelData.bundleMaterialVisible

    MouseArea {
        id: mouseArea

        enabled: root.downloadState !== "downloading"
        hoverEnabled: true
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton | Qt.RightButton

        onPressed: (mouse) => {
            if (mouse.button === Qt.LeftButton && !materialsModel.importerRunning) {
                if (root.downloadState === "downloaded")
                    ContentLibraryBackend.rootView.startDragMaterial(modelData, mapToGlobal(mouse.x, mouse.y))
            } else if (mouse.button === Qt.RightButton && root.downloadState === "downloaded") {
                root.showContextMenu()
            }
        }
    }

    Column {
        anchors.fill: parent
        spacing: 1

        DownloadPane {
            id: downloadPane
            width: root.width - 10
            height: img.width
            visible: root.downloadState === "downloading"

            onRequestCancel: downloader.cancel()
        }

        Image {
            id: img

            width: root.width
            height: img.width
            anchors.horizontalCenter: parent.horizontalCenter
            source: modelData.bundleMaterialIcon
            cache: false
            visible: root.downloadState != "downloading"

            Rectangle { // circular indicator for imported bundle materials
                width: 10
                height: 10
                radius: 5
                anchors.right: img.right
                anchors.top: img.top
                anchors.margins: 5
                color: "#00ff00"
                border.color: "#555555"
                border.width: 1
                visible: modelData.bundleMaterialImported

                ToolTip {
                    visible: indicatorMouseArea.containsMouse
                    text: qsTr("Material is imported to project")
                    delay: 1000
                }

                MouseArea {
                    id: indicatorMouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                }
            }

            IconButton {
                icon: StudioTheme.Constants.plus
                tooltip: qsTr("Add an instance to project")
                buttonSize: 22
                property color c: "white"
                normalColor: Qt.hsla(c.hslHue, c.hslSaturation, c.hslLightness, .2)
                hoverColor: Qt.hsla(c.hslHue, c.hslSaturation, c.hslLightness, .3)
                pressColor: Qt.hsla(c.hslHue, c.hslSaturation, c.hslLightness, .4)
                anchors.right: img.right
                anchors.bottom: img.bottom
                enabled: !ContentLibraryBackend.materialsModel.importerRunning
                visible: root.downloadState === "downloaded"
                         && (containsMouse || mouseArea.containsMouse)

                onClicked: {
                    ContentLibraryBackend.materialsModel.addToProject(modelData)
                }
            }

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

                tooltip: qsTr("Click to download material")
                buttonSize: 22

                transparentBg: true
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                visible: root.downloadState !== "downloaded"

                anchors.bottomMargin: 0
                anchors.rightMargin: 4

                Rectangle { // arrow fill
                    anchors.centerIn: parent
                    z: -1

                    width: parent.width / 2
                    height: parent.height / 2
                    color: "black"
                }

                onClicked: {
                    if (root.downloadState !== "" && root.downloadState !== "failed")
                        return

                    downloadPane.beginDownload(Qt.binding(function() { return downloader.progress }))

                    root.downloadState = ""
                    downloader.start()
                }
            }
        }

        Text {
            id: matName

            width: img.width
            anchors.horizontalCenter: parent.horizontalCenter
            horizontalAlignment: TextInput.AlignHCenter

            text: modelData.bundleMaterialName
            elide: Text.ElideRight
            font.pixelSize: StudioTheme.Values.myFontSize
            color: StudioTheme.Values.themeTextColor
        }
    }

    Timer {
        id: delayedFinish
        interval: 200

        onTriggered: {
            downloadPane.endDownload()

            root.downloadState = "downloaded"
        }
    }

    MultiFileDownloader {
        id: downloader

        baseUrl: modelData.bundleMaterialBaseWebUrl
        files: modelData.bundleMaterialFiles

        targetDirPath: modelData.bundleMaterialParentPath

        onDownloadStarting: {
            root.downloadState = "downloading"
        }

        onFinishedChanged: {
            delayedFinish.restart()
        }

        onDownloadCanceled: {
            downloadPane.endDownload()

            root.downloadState = ""
        }

        onDownloadFailed: {
            downloadPane.endDownload()

            root.downloadState = "failed"
        }

        downloader: FileDownloader {
            id: fileDownloader
            url: downloader.nextUrl
            probeUrl: false
            downloadEnabled: true
            targetFilePath: downloader.nextTargetPath
        }
    }
}
