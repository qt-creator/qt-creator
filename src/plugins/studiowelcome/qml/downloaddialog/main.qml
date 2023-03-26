// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.0
import QtQuick.Controls 2.15

import ExampleCheckout 1.0
import QtQuick.Layouts 1.11

import StudioFonts 1.0
import StudioTheme 1.0


Rectangle {

    property color currentThemeBackground: Values.welcomeScreenBackground
    property color themeTextColor: Values.themeTextColor

    id: root
    property alias url: downloader.url
    property string path: fileExtractor.targetPath
    width: 620
    height: 300

    color: root.currentThemeBackground

    property color textColor: Values.themeTextColor

    signal canceled
    signal accepted

    property string tempFile
    property string completeBaseName

    StackLayout {
        id: stackLayout
        anchors.fill: parent
        currentIndex: root.tempFile.length === 0 ? 1 : 1

        FileExtractor {
            id: fileExtractor
            archiveName: root.completeBaseName.length === 0 ? downloader.completeBaseName : root.completeBaseName
            sourceFile: root.tempFile.length === 0 ? downloader.outputFile : root.tempFile
        }

        FileDownloader {
            id: downloader
            //onNameChanged: start()
            onFinishedChanged: {
                button.enabled = downloader.finished
                if (!downloader.finished)
                    stackLayout.currentIndex = 3
            }

            onDownloadFailed:  stackLayout.currentIndex = 3
        }

        Item {
            id: download
            Layout.fillHeight: true
            Layout.fillWidth: true

            PushButton {
                id: button
                x: 532
                y: 432
                text: qsTr("Continue")
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 20
                anchors.rightMargin: 20
                enabled: false
                onClicked: stackLayout.currentIndex = 1

            }

            CoolProgressBar {
                id: coolProgressBar
                width: 605
                anchors.top: parent.top
                value: downloader.progress
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.topMargin: 69
            }

            DialogLabel {
                x: 201
                text: "Downloading Example " + downloader.completeBaseName
                anchors.top: parent.top
                anchors.topMargin: 22
                anchors.horizontalCenter: parent.horizontalCenter
            }

            PushButton{
                id: downloadbutton
                y: 420
                enabled: !button.enabled
                text: qsTr("Start Download")
                anchors.left: parent.left
                anchors.bottom: parent.bottom
                anchors.leftMargin: 20
                anchors.bottomMargin: 20
                onClicked: {
                    downloadbutton.enabled = false
                    downloader.start()
                }

            }

            CircularIndicator {
                id: circularIndicator
                x: 304
                anchors.top: parent.top
                anchors.horizontalCenterOffset: 0
                value: downloader.progress
                anchors.topMargin: 120
                anchors.horizontalCenter: parent.horizontalCenter
            }

        }

        Item {
            id: destiationfolder
            Layout.fillHeight: true
            Layout.fillWidth: true

            PushButton {
                id: nextPageDestination
                x: 532
                y: 432
                text: qsTr("Continue")
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                enabled: !fileExtractor.targetFolderExists
                anchors.bottomMargin: 20
                anchors.rightMargin: 20
                onClicked: {
                    stackLayout.currentIndex = 2
                    fileExtractor.extract()
                }

            }

            RowLayout {
                y: 114
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.rightMargin: 104
                anchors.leftMargin: 67

                TextField {
                    id: textField
                    text: fileExtractor.targetPath
                    Layout.fillWidth: true
                    font.family: StudioFonts.titilliumWeb_light
                    wrapMode: Text.WordWrap
                    selectByMouse: true
                    readOnly: true
                }

                PushButton{
                    id: browse
                    text: qsTr("Browse")
                    onClicked: fileExtractor.browse()
                }
            }

            DialogLabel {
                id: label
                y: 436
                text: qsTr("Folder ") + downloader.completeBaseName + (" already exists")
                anchors.left: parent.left
                anchors.bottom: parent.bottom
                anchors.leftMargin: 20
                anchors.bottomMargin: 20
                visible: !nextPageDestination.enabled
            }

            PushButton{
                id: button5
                x: 400
                y: 420
                text: qsTr("Cancel")
                anchors.right: nextPageDestination.left
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 20
                anchors.rightMargin: 20
                onClicked: root.canceled()

            }

            DialogLabel {
                text: "Choose installation folder"
                anchors.top: parent.top
                anchors.topMargin: 22
                anchors.horizontalCenter: parent.horizontalCenter
                x: 8
            }
        }

        Item {
            id: extraction
            Layout.fillHeight: true
            Layout.fillWidth: true

            PushButton{
                id: done
                x: 532
                y: 432
                text: qsTr("Open")
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 20
                anchors.rightMargin: 20
                enabled: fileExtractor.finished
                onClicked: root.accepted()

            }


            DialogLabel {
                id: text2
                text: fileExtractor.count + " files " + (fileExtractor.size / 1024 / 1024).toFixed(2) + " MB "+ fileExtractor.currentFile
                anchors.left: parent.left
                anchors.bottom: parent.bottom
                font.pixelSize: 12
                wrapMode: Text.WrapAnywhere
                anchors.leftMargin: 20
                anchors.bottomMargin: 20
            }

            PushButton{
                id: details
                x: 8
                text: qsTr("Details")
                anchors.top: parent.top
                anchors.topMargin: 66
                anchors.horizontalCenter: parent.horizontalCenter
                checkable: true

            }


            DialogLabel {
                x: 8
                text: "Extracting Example " + downloader.completeBaseName
                anchors.top: parent.top
                anchors.topMargin: 22
                anchors.horizontalCenter: parent.horizontalCenter
            }

            Flickable {
                visible: details.checked
                clip: true
                anchors.bottomMargin: 60
                anchors.rightMargin: 20
                anchors.leftMargin: 20
                anchors.topMargin: 120
                anchors.fill: parent
                id: flickable
                interactive: false

                DialogLabel {
                    onHeightChanged: flickable.contentY =  text1.implicitHeight - flickable.height
                    id: text1

                    text: fileExtractor.detailedText

                    font.pixelSize: 12
                    wrapMode: Text.WrapAnywhere

                    width: flickable.width
                }
            }
        }

        Item {
            id: failed
            Layout.fillHeight: true
            Layout.fillWidth: true

            PushButton{
                id: finish
                x: 532
                y: 432
                text: qsTr("Finish")
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 20
                anchors.rightMargin: 20
                onClicked: root.canceled()

            }

            DialogLabel {
                x: 8
                text: qsTr("Download failed")
                anchors.top: parent.top
                anchors.topMargin: 22
                anchors.horizontalCenter: parent.horizontalCenter
            }
        }
    }
}
