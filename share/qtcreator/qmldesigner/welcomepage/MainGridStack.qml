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

import QtQuick
import QtQuick.Controls
import WelcomeScreen 1.0
import QtQuick.Layouts
import projectmodel 1.0

Item {
    id: thumbnails
    width: 1500
    height: 800
    clip: true
    property alias listStackLayoutCurrentIndex: listStackLayout.currentIndex
    property alias stackLayoutCurrentIndex: gridStackLayout.currentIndex

    property var projectModel: Constants.projectModel

    Rectangle {
        id: thumbnailGridBack
        color: Constants.currentThumbnailGridBackground
        anchors.fill: parent

        StackLayout {
            id: gridStackLayout
            visible: !Constants.isListView
            anchors.fill: parent
            currentIndex: 0

            CustomGrid {
                id: myRecentGrid
                x: 20
                y: 20
                anchors.fill: parent
                Layout.fillHeight: true
                Layout.fillWidth: true
                anchors.rightMargin: 20
                anchors.leftMargin: 20
                anchors.bottomMargin: 20
                anchors.topMargin: 45
                mymodel: thumbnails.projectModel
                myDelegate: ThumbnailDelegate {
                    tagged: false
                    likable: false
                    downloadable: false
                    hasPath: true
                    hasDescription: false
                    hasPrice: false
                    thumbnailPlaceholderSource: "images/newThumbnail.png"
                    onClicked: projectModel.openProjectAt(index)
                }
            }

            CustomGrid {
                id: myExamplesGrid
                x: 20
                y: 20
                anchors.fill: parent
                anchors.rightMargin: 20
                anchors.leftMargin: 20
                anchors.bottomMargin: 20
                anchors.topMargin: 45
                mymodel: ExamplesModel {}
                myDelegate: ThumbnailDelegate {
                    tagged: false
                    likable: false
                    downloadable: showDownload
                    hasPath: false
                    hasDescription: false
                    hasPrice: false
                    onClicked: projectModel.openExample(projectName,
                                                        qmlFileName, url,
                                                        explicitQmlproject)
                }
            }

            CustomGrid {
                id: myTutorialsGrid
                x: 20
                y: 20
                anchors.fill: parent
                anchors.rightMargin: 20
                anchors.leftMargin: 20
                anchors.bottomMargin: 20
                anchors.topMargin: 45
                mymodel: TutorialsModel {}
                myDelegate: ThumbnailDelegate {
                    tagged: false
                    likable: false
                    downloadable: false
                    hasPath: false
                    hasDescription: false
                    hasPrice: false
                    onClicked: Qt.openUrlExternally(url)
                }
            }

            CustomGrid {
                id: myMarketplaceGrid
                x: 20
                y: 20
                anchors.fill: parent
                anchors.rightMargin: 20
                anchors.leftMargin: 20
                anchors.bottomMargin: 20
                anchors.topMargin: 45
                mymodel: thumbnails.projectModel
                myDelegate: ThumbnailDelegate {
                    tagged: true
                    likable: false
                    downloadable: false
                    hasDescription: true
                    hasPath: false
                    hasPrice: true
                }
            }
        }

        StackLayout {
            id: listStackLayout
            visible: Constants.isListView
            anchors.fill: parent
            currentIndex: 0

            CustomGrid {
                id: myRecentList
                anchors.fill: parent
                mymodel: thumbnails.projectModel
                anchors.topMargin: 45
                myDelegate: MyRecentThumbnailDelegate {}
                anchors.leftMargin: 20
                Layout.fillWidth: true
                anchors.rightMargin: 20
                Layout.fillHeight: true
                anchors.bottomMargin: 20
            }

            CustomGrid {
                id: myExamplesList
                x: 20
                y: 20
                anchors.fill: parent
                anchors.bottomMargin: 20
                mymodel: ExamplesModel {}
                anchors.topMargin: 45
                myDelegate: MyExampleThumbnailDelegate {}
                anchors.leftMargin: 20
                anchors.rightMargin: 20
            }
        }
    }
}

/*##^##
Designer {
    D{i:0;formeditorZoom:0.66;height:838;width:1509}
}
##^##*/

