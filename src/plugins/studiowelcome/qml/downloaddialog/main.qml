/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

import QtQuick 2.0
import QtQuick.Controls 2.15

import ExampleCheckout 1.0
import QtQuick.Layouts 1.11

import StudioFonts 1.0

Rectangle {
    id: root
    property alias url: downloader.url
    property string path: fileExtractor.targetPath
    width: 620
    height: 300

    color: "#2d2e30"

    property color textColor: "#b9b9ba"

    signal canceled
    signal accepted

    StackLayout {
        id: stackLayout
        anchors.fill: parent
        currentIndex: 0

        FileExtractor {
            id: fileExtractor
            sourceFile: downloader.tempFile
            archiveName: downloader.completeBaseName
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

            DialogButton {
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

            DialogButton {
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

            DialogButton {
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

                DialogButton {
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

            DialogButton {
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


            DialogButton {
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


            DialogButton {
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

            DialogButton {
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
