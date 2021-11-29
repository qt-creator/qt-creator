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

Item {
    id: thumbnailDelegate
    width: 220
    height: 220
    property bool hasPrice: true
    property bool hasDescription: true
    property bool hasPath: true
    property bool likable: true
    property bool downloadable: true
    property bool tagged: true
    property alias projectNameLabelText: projectNameLabel.text
    property alias projectPathLabelText: projectPathLabel.text
    property alias thumbnailPlaceholderSource: thumbnailPlaceholder.source
    state: "normal"

    signal clicked()

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        propagateComposedEvents: true
        hoverEnabled: true
        onClicked: thumbnailDelegate.clicked()
    }

    Rectangle {
        id: recentThumbBackground
        x: 0
        y: 0
        width: 220
        height: 220
        color: "#ea8c8c"
        border.color: "#00000000"

        Image {
            id: thumbnailPlaceholder
            height: 126
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            source: typeof(thumbnail) === "undefined" ? "" : thumbnail
            anchors.rightMargin: 8
            anchors.leftMargin: 10
            anchors.topMargin: 8
            fillMode: Image.PreserveAspectFit

            Rectangle {
                id: rectangle
                x: 10
                y: 8
                opacity: 0.903
                color: Constants.currentNormalThumbnailBackground
                border.color: "#00323232"
                anchors.fill: parent

                Text {
                    id: text1
                    x: 5
                    y: 5
                    color: Constants.currentGlobalText
                    text: qsTr("Description Place Holder Text For marketplace icons")
                    anchors.fill: parent
                    font.pixelSize: 12
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    wrapMode: Text.WordWrap
                    anchors.rightMargin: 5
                    anchors.leftMargin: 5
                    anchors.bottomMargin: 5
                    anchors.topMargin: 5
                }
            }
        }
    }


    Rectangle {
        id: projectNameBackground
        x: 10
        y: 139
        width: 202
        height: 30
        color: "#e5b0e4"

        Text {
            id: projectNameLabel
            color: Constants.currentGlobalText
            text: typeof(displayName) === "undefined" ? projectName : displayName
            elide: Text.ElideRight
            anchors.fill: parent
            font.pixelSize: 12
            verticalAlignment: Text.AlignVCenter
            anchors.bottomMargin: 0
            anchors.leftMargin: 0
            anchors.topMargin: 0
            clip: true
            leftPadding: 5
            anchors.rightMargin: 35
        }

        RowLayout {
            id: downloadFavorite
            x: 140
            y: 0
            width: 62
            height: 30
            layoutDirection: Qt.RightToLeft

            DownloadButton {
                id: downloadButton
                visible: thumbnailDelegate.downloadable
            }

            FavoriteToggle {
                id: myFavoriteToggle
                visible: thumbnailDelegate.likable
                Layout.preferredHeight: 30
                Layout.preferredWidth: 27
            }
        }
    }


    Text {
        id: projectPathLabel
        x: 10
        y: 176
        width: 177
        height: 15
        visible: thumbnailDelegate.hasPath
        color: Constants.currentBrand
        text: typeof(prettyFilePath) === "undefined" ? "" : prettyFilePath
        elide: Text.ElideLeft
        font.pixelSize: 12
        leftPadding: 5
        renderType: Text.NativeRendering
    }


    TagArea {
        id: tagArea
        y: 197
        height: 15
        visible: thumbnailDelegate.tagged
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 8
        anchors.rightMargin: 3
        anchors.leftMargin: 3
    }

    Text {
        id: price
        x: 10
        y: 176
        width: 202
        height: 15
        visible: thumbnailDelegate.hasPrice
        color: Constants.currentGlobalText
        text: "€249.99 - Yearly"
        font.pixelSize: 12
        horizontalAlignment: Text.AlignRight
    }


    states: [
        State {
            name: "normal"
            when: !mouseArea.containsMouse && !mouseArea.pressed && !myFavoriteToggle.isHovered && !downloadButton.isHovered && (thumbnailDelegate.hasDescription || !thumbnailDelegate.hasDescription)

            PropertyChanges {
                target: recentThumbBackground
                color: Constants.currentNormalThumbnailBackground
                border.color: "#002f779f"
            }

            PropertyChanges {
                target: projectNameBackground
                color: Constants.currentNormalThumbnailLabelBackground
                border.color: "#00000000"
            }

            PropertyChanges {
                target: thumbnailPlaceholder
                opacity: 0.528
                visible: true
            }

            PropertyChanges {
                target: rectangle
                visible: false
            }
        },
        State {
            name: "hover"
            when: ((mouseArea.containsMouse || myFavoriteToggle.isHovered || downloadButton.isHovered) && !mouseArea.pressed && !thumbnailDelegate.hasDescription)

            PropertyChanges {
                target: recentThumbBackground
                color: Constants.currentHoverThumbnailBackground
                border.color: "#002f779f"
            }

            PropertyChanges {
                target: projectNameBackground
                color: Constants.currentHoverThumbnailLabelBackground
                border.color: "#00000000"
            }

            PropertyChanges {
                target: thumbnailPlaceholder
                visible: true
            }

            PropertyChanges {
                target: rectangle
                visible: false
            }
        },
        State {
            name: "press"
            when: (mouseArea.containsMouse || !mouseArea.containsMouse) && mouseArea.pressed && !thumbnailDelegate.hasDescription && !thumbnailDelegate.hasDescription

            PropertyChanges {
                target: recentThumbBackground
                color: Constants.currentHoverThumbnailBackground
                border.color: Constants.currentBrand
                border.width: 2
            }

            PropertyChanges {
                target: projectNameBackground
                color: Constants.currentBrand
                border.color: "#00000000"
            }

            PropertyChanges {
                target: thumbnailPlaceholder
                visible: true
            }

            PropertyChanges {
                target: rectangle
                visible: false
            }

            PropertyChanges {
                target: projectNameLabel
                color: Constants.currentActiveGlobalText
            }
        },
        State {
            name: "hoverDescription"
            when: mouseArea.containsMouse && !mouseArea.pressed && thumbnailDelegate.hasDescription
            PropertyChanges {
                target: recentThumbBackground
                color: Constants.currentHoverThumbnailBackground
                border.color: "#002f779f"
            }

            PropertyChanges {
                target: projectNameBackground
                color: Constants.currentHoverThumbnailLabelBackground
                border.color: "#00000000"
            }

            PropertyChanges {
                target: thumbnailPlaceholder
                visible: true
            }

            PropertyChanges {
                target: rectangle
                visible: true
            }
        },
        State {
            name: "pressDescription"
            when: (mouseArea.containsMouse || !mouseArea.containsMouse) && mouseArea.pressed && thumbnailDelegate.hasDescription
            PropertyChanges {
                target: recentThumbBackground
                color: Constants.currentHoverThumbnailBackground
                border.color: Constants.currentBrand
                border.width: 2
            }

            PropertyChanges {
                target: projectNameBackground
                color: Constants.currentBrand
                border.color: "#00000000"
            }

            PropertyChanges {
                target: thumbnailPlaceholder
                visible: true
            }

            PropertyChanges {
                target: rectangle
                visible: true
            }
        }
    ]
}

/*##^##
Designer {
    D{i:0;formeditorZoom:3}
}
##^##*/
