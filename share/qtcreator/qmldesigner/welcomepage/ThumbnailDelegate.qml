// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import WelcomeScreen 1.0
import ExampleCheckout 1.0

Item {
    id: root
    width: Constants.thumbnailSize
    height: Constants.thumbnailSize
    clip: true
    state: "normal"

    property bool textWrapped: false
    property bool hasDescription: false
    property bool hasPath: false
    property bool hasUpdate: false
    property bool downloadable: false
    property int numberOfPanels: 3

    enum Type {
        RecentProject,
        Example,
        Tutorial
    }

    property int type: ThumbnailDelegate.Type.RecentProject

    property alias projectNameLabelText: projectNameLabel.text
    property alias projectPathLabelText: projectPathLabel.text
    property alias thumbnailPlaceholderSource: thumbnailPlaceholder.source

    property alias downloadUrl: downloader.url
    property alias tempFile: downloader.outputFile
    property alias completeBaseName: downloader.completeBaseName
    property alias targetPath: extractor.targetPath

    property alias bannerLabelText: updateText.text

    enum Panel {
        InBetween,
        Download,
        Main,
        Details
    }

    property int currentPanel: ThumbnailDelegate.Panel.Main

    signal clicked()
    signal rightClicked()

    function startDownload() {
        twirlToDownloadAnimation.start()
        downloadPanel.text = downloadPanel.downloadText
        downloadPanel.allowCancel = true
        downloadPanel.value = Qt.binding(function() { return downloader.progress })
        downloader.start()
        mouseArea.enabled = false
    }

    function thumbnailClicked() {
        if (root.type === ThumbnailDelegate.Type.RecentProject
            || root.type === ThumbnailDelegate.Type.Tutorial)
            root.clicked() // open recent project/tutorial

        if (Constants.loadingProgress < 95)
            return
        if (root.type === ThumbnailDelegate.Type.Example) {
            if (root.downloadable
                && !downloadButton.alreadyDownloaded
                && !downloadButton.downloadUnavailable) {
                root.startDownload()
            } else if (downloadButton.alreadyDownloaded) {
                root.clicked() // open example
            }
        }
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        propagateComposedEvents: true
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton | Qt.RightButton

        Connections {
            target: mouseArea
            function onClicked(mouse) {
                if (mouse.button === Qt.RightButton)
                    root.rightClicked()
                else
                    root.thumbnailClicked(mouse)
            }
        }
    }

    CustomDialog {
        id: overwriteDialog
        title: qsTr("Overwrite Example?")
        standardButtons: Dialog.Yes | Dialog.No
        modal: true
        anchors.centerIn: parent

        onAccepted: root.startDownload()

        Text {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            font.pixelSize: 12
            color: Constants.currentGlobalText
            text: qsTr("Example already exists.<br>Do you want to replace it?")
        }
    }

    Item {
        id: canvas
        width: Constants.thumbnailSize
        height: Constants.thumbnailSize * root.numberOfPanels

        DownloadPanel {
            id: downloadPanel

            height: Constants.thumbnailSize
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: mainPanel.top
            radius: 10
            value: downloader.progress

            readonly property string downloadText: qsTr("Downloading...")
            readonly property string extractText: qsTr("Extracting...")

            onCancelRequested: downloader.cancel()
        }

        Rectangle {
            id: mainPanel
            height: Constants.thumbnailSize
            anchors.left: parent.left
            anchors.right: parent.right
            color: "#ea8c8c"
            radius: 10

            property bool multiline: (projectNameLabelMetric.width >= projectNameLabel.width)

            TextMetrics {
                id: projectNameLabelMetric
                text: projectNameLabel.text
                font: projectNameLabel.font
            }

            Image {
                id: thumbnailPlaceholder
                anchors.fill: parent
                anchors.bottomMargin: Constants.imageBottomMargin
                anchors.rightMargin: Constants.thumbnailMargin
                anchors.leftMargin: Constants.thumbnailMargin
                anchors.topMargin: Constants.thumbnailMargin
                fillMode: Image.PreserveAspectFit
                verticalAlignment: Image.AlignTop
                mipmap: true
            }

            Rectangle {
                id: projectNameBackground
                height: mainPanel.multiline && !root.hasPath ? Constants.titleHeightMultiline
                                                             : Constants.titleHeight
                color: "#e5b0e4"
                radius: 3
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: thumbnailPlaceholder.bottom
                anchors.topMargin: Constants.titleBackgroundTopMargin
                anchors.leftMargin: Constants.thumbnailMargin
                anchors.rightMargin: Constants.thumbnailMargin

                Text {
                    id: projectNameLabel
                    color: Constants.currentGlobalText
                    text: typeof(displayName) === "undefined" ? projectName : displayName
                    elide: root.hasPath ? Text.ElideMiddle : Text.ElideRight
                    font.pixelSize: 16
                    wrapMode: Text.WordWrap
                    maximumLineCount: 2
                    clip: false
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.leftMargin: Constants.titleMargin
                    anchors.rightMargin: Constants.titleMargin

                    MouseArea {
                        id: projectNameMouseArea
                        anchors.fill: parent
                        hoverEnabled: true
                        // Only enable the MouseArea if label actually contains text
                        enabled: projectNameLabel.text !== "" ? true : false

                        Connections {
                            target: projectNameMouseArea
                            onClicked: root.thumbnailClicked()
                        }
                    }

                    ToolTip {
                        id: projectNameToolTip
                        y: -projectNameToolTip.height
                        visible: projectNameMouseArea.containsMouse && projectNameLabel.truncated
                        text: projectNameLabel.text
                        delay: 1000
                        height: 20
                        background: Rectangle {
                            color: Constants.currentToolTipBackground
                            border.color: Constants.currentToolTipOutline
                            border.width: 1
                        }
                        contentItem: Text {
                            color: Constants.currentToolTipText
                            text: projectNameToolTip.text
                            verticalAlignment: Text.AlignVCenter
                        }
                    }
                }

                DownloadButton {
                    id: downloadButton

                    anchors.right: parent.right
                    anchors.top: parent.top
                    visible: root.downloadable
                    enabled: !downloadButton.downloadUnavailable

                    globalHover: (mouseArea.containsMouse || projectNameMouseArea.containsMouse)
                                 && !downloadButton.alreadyDownloaded
                                 && !downloadButton.downloadUnavailable
                                 && !downloadButton.updateAvailable

                    alreadyDownloaded: extractor.targetFolderExists
                    downloadUnavailable: !downloader.available
                    updateAvailable: downloader.lastModified > extractor.birthTime

                    FileDownloader {
                        id: downloader
                        downloadEnabled: $dataModel.downloadEnabled()
                        probeUrl: true

                        onFinishedChanged: {
                            downloadPanel.text = downloadPanel.extractText
                            downloadPanel.allowCancel = false
                            downloadPanel.value = Qt.binding(function() { return extractor.progress })
                            extractor.extract()
                        }

                        onDownloadStarting: {
                            $dataModel.usageStatisticsDownloadExample(downloader.name)
                        }

                        onDownloadCanceled: {
                            downloadPanel.text = ""
                            downloadPanel.value = 0
                            twirlToMainAnimation.start()
                            mouseArea.enabled = true
                        }
                    }

                    FileExtractor {
                        id: extractor
                        archiveName: downloader.completeBaseName
                        sourceFile: downloader.outputFile
                        targetPath: $dataModel.targetPath()
                        alwaysCreateDir: true
                        clearTargetPathContents: true
                        onFinishedChanged: {
                            twirlToMainAnimation.start()
                            root.bannerLabelText = qsTr("Recently Downloaded")
                            mouseArea.enabled = true
                        }
                    }

                    Connections {
                        target: downloadButton
                        onDownloadClicked: {
                            if (downloadButton.alreadyDownloaded) {
                                overwriteDialog.open()
                            } else {
                                if (downloadButton.enabled)
                                    root.startDownload()
                            }
                        }
                    }

                    Connections {
                        target: $dataModel
                        onTargetPathMustChange: (path) => {
                            extractor.changeTargetPath(path)
                        }
                    }
                }
            }

            Text {
                id: projectPathLabel
                visible: root.hasPath
                color: Constants.currentBrand
                text: typeof(prettyFilePath) === "undefined" ? "" : prettyFilePath
                elide: Text.ElideLeft
                renderType: Text.NativeRendering
                font.pixelSize: 16
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: projectNameBackground.bottom
                anchors.topMargin: 5
                anchors.rightMargin: Constants.thumbnailMargin
                anchors.leftMargin: Constants.thumbnailMargin
                leftPadding: 5

                MouseArea {
                    id: projectPathMouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    // Only enable the MouseArea if label actually contains text
                    enabled: projectPathLabel.text !== ""
                }

                ToolTip {
                    id: projectPathToolTip
                    y: -projectPathToolTip.height
                    visible: projectPathMouseArea.containsMouse && projectPathLabel.truncated
                    text: projectPathLabel.text
                    delay: 1000
                    height: 20
                    background: Rectangle {
                        color: Constants.currentToolTipBackground
                        border.color: Constants.currentToolTipOutline
                        border.width: 1
                    }
                    contentItem: Text {
                        color: Constants.currentToolTipText
                        text: projectPathToolTip.text
                        verticalAlignment: Text.AlignVCenter
                    }
                }
            }

            Rectangle {
                id: updateBackground
                width: 200
                height: 25
                visible: root.bannerLabelText !== ""
                color: Constants.currentBrand
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.topMargin: 0
                anchors.rightMargin: Constants.thumbnailMargin
                anchors.leftMargin: Constants.thumbnailMargin

                Text {
                    id: updateText
                    color: Constants.darkActiveGlobalText
                    text: typeof(displayName) === "bannerText" ? "" : bannerText
                    anchors.fill: parent
                    font.pixelSize: 12
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }
        }

        Rectangle {
            id: detailsPanel
            height: Constants.thumbnailSize
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: mainPanel.bottom
            color: Constants.currentNormalThumbnailBackground
            radius: 10

            Text {
                id: recentProjectInfo
                color: Constants.currentGlobalText
                text: typeof(description) === "undefined" ? "" : description
                anchors.fill: parent
                font.pixelSize: 12
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignTop
                wrapMode: Text.WordWrap
                anchors.margins: Constants.thumbnailMargin
                anchors.topMargin: 25
            }

            TagArea {
                id: tagArea
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.bottomMargin: Constants.thumbnailMargin
                anchors.rightMargin: Constants.thumbnailMargin
                anchors.leftMargin: Constants.thumbnailMargin
                tags: typeof(tagData) === "undefined" ? "" : tagData.split(",")
            }
        }
    }

    states: [
        State {
            name: "normal"
            when: !mouseArea.containsMouse && !(mouseArea.pressedButtons & Qt.LeftButton)
                  && !projectPathMouseArea.containsMouse && !projectNameMouseArea.containsMouse
                  && !downloadButton.isHovered && !twirlButtonDown.isHovered
                  && !twirlButtonUp.isHovered

            PropertyChanges {
                target: mainPanel
                color: Constants.currentNormalThumbnailBackground
                border.width: 0
            }
            PropertyChanges {
                target: detailsPanel
                color: Constants.currentNormalThumbnailBackground
                border.width: 0
            }
            PropertyChanges {
                target: projectNameBackground
                color: Constants.currentNormalThumbnailLabelBackground
            }
            PropertyChanges {
                target: twirlButtonDown
                parentHovered: false
            }
            PropertyChanges {
                target: twirlButtonUp
                parentHovered: false
            }
        },
        State {
            name: "hover"
            when: (mouseArea.containsMouse || projectPathMouseArea.containsMouse
                   || projectNameMouseArea.containsMouse || downloadButton.isHovered
                   || twirlButtonDown.isHovered || twirlButtonUp.isHovered)
                  && !(mouseArea.pressedButtons & Qt.LeftButton) && !root.hasDescription

            PropertyChanges {
                target: mainPanel
                color: Constants.currentHoverThumbnailBackground
                border.width: 0
            }
            PropertyChanges {
                target: detailsPanel
                color: Constants.currentHoverThumbnailBackground
                border.width: 0
            }
            PropertyChanges {
                target: projectNameBackground
                color: Constants.currentHoverThumbnailLabelBackground
            }
            PropertyChanges {
                target: thumbnailPlaceholder
                visible: true
            }
            PropertyChanges {
                target: twirlButtonDown
                parentHovered: true
            }
            PropertyChanges {
                target: twirlButtonUp
                parentHovered: true
            }
        },
        State {
            name: "press"
            when: (mouseArea.pressedButtons & Qt.LeftButton) && !root.hasDescription

            PropertyChanges {
                target: mainPanel
                color: Constants.currentHoverThumbnailBackground
                border.color: Constants.currentBrand
                border.width: 2
            }
            PropertyChanges {
                target: detailsPanel
                color: Constants.currentHoverThumbnailBackground
                border.color: Constants.currentBrand
                border.width: 2
            }
            PropertyChanges {
                target: projectNameBackground
                color: Constants.currentBrand
            }
            PropertyChanges {
                target: thumbnailPlaceholder
                visible: true
            }
            PropertyChanges {
                target: projectNameLabel
                color: Constants.darkActiveGlobalText
            }
        },
        State {
            name: "hoverDescription"
            when: mouseArea.containsMouse && !(mouseArea.pressedButtons & Qt.LeftButton)
                  && root.hasDescription

            PropertyChanges {
                target: mainPanel
                color: Constants.currentHoverThumbnailBackground
                border.width: 0
            }
            PropertyChanges {
                target: projectNameBackground
                color: Constants.currentHoverThumbnailLabelBackground
            }
            PropertyChanges {
                target: thumbnailPlaceholder
                visible: true
            }
        },
        State {
            name: "pressDescription"
            when: (mouseArea.pressedButtons & Qt.LeftButton) && root.hasDescription

            PropertyChanges {
                target: mainPanel
                color: Constants.currentHoverThumbnailBackground
                border.color: Constants.currentBrand
                border.width: 2
            }
            PropertyChanges {
                target: projectNameBackground
                color: Constants.currentBrand
            }
            PropertyChanges {
                target: thumbnailPlaceholder
                visible: true
            }
        }
    ]

    TwirlButton {
        id: twirlButtonDown
        height: 20
        visible: root.currentPanel === ThumbnailDelegate.Panel.Main
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom

        Connections {
            target: twirlButtonDown
            onTriggerRelease: twirlToDetailsAnimation.start()
        }
    }

    TwirlButton {
        id: twirlButtonUp
        height: 20
        visible: root.currentPanel === ThumbnailDelegate.Panel.Details
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        rotation: 180

        Connections {
            target: twirlButtonUp
            onTriggerRelease: twirlToMainAnimation.start()
        }
    }

    NumberAnimation {
        id: twirlToDetailsAnimation
        target: canvas
        property: "y"
        easing.bezierCurve: [0.972,-0.176,0.0271,1.16,1,1]
        duration: 250
        alwaysRunToEnd: true
        to: -Constants.thumbnailSize // dynamic size because of rescale - needs to be inverted because animation goes into negative range
        from: canvas.y
    }

    NumberAnimation {
        id: twirlToMainAnimation
        target: canvas
        property: "y"
        easing.bezierCurve: [0.972,-0.176,0.0271,1.16,1,1]
        alwaysRunToEnd: true
        duration: 250
        to: 0
        from: canvas.y
    }

    NumberAnimation {
        id: twirlToDownloadAnimation
        target: canvas
        property: "y"
        easing.bezierCurve: [0.972,-0.176,0.0271,1.16,1,1]
        alwaysRunToEnd: true
        duration: 250
        to: Constants.thumbnailSize
        from: canvas.y
    }

    Connections {
        target: twirlToDetailsAnimation
        onStarted: root.currentPanel = ThumbnailDelegate.Panel.InBetween
        onFinished: {
            root.currentPanel = ThumbnailDelegate.Panel.Details
            canvas.y = Qt.binding(function() {return -Constants.thumbnailSize })
        }
    }

    Connections {
        target: twirlToMainAnimation
        onStarted: root.currentPanel = ThumbnailDelegate.Panel.InBetween
        onFinished: {
            root.currentPanel = ThumbnailDelegate.Panel.Main
            canvas.y = Qt.binding(function() {return 0 })
        }
    }

    Connections {
        target: twirlToDownloadAnimation
        onStarted: root.currentPanel = ThumbnailDelegate.Panel.InBetween
        onFinished: {
            root.currentPanel = ThumbnailDelegate.Panel.Download
            canvas.y = Qt.binding(function() {return Constants.thumbnailSize })
        }
    }
}
