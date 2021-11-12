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
import QtQuick.Layouts 1.0

Item {
    id: exampleThumbnailDelegate
    width: 220
    height: 220
    property alias projectNameLabelText: projectNameLabel.text
    property alias projectPathLabelText: projectPathLabel.text
    property alias thumbnailPlaceholderSource: thumbnailPlaceholder.source
    state: "normal"

    Rectangle {
        id: recentThumbBackground
        x: 0
        y: 0
        width: 220
        height: 220
        color: "#d0cfcf"
        border.color: "#00000000"

        Image {
            id: thumbnailPlaceholder
            height: 126
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            source: thumbnail
            anchors.rightMargin: 8
            anchors.leftMargin: 10
            anchors.topMargin: 8
            fillMode: Image.PreserveAspectFit
        }
    }

    Rectangle {
        id: projectNameBackground
        x: 10
        y: 144
        width: 202
        height: 30
        color: "#ffffff"

        Text {
            id: projectNameLabel
            color: Constants.currentGlobalText
            text: displayName
            elide: Text.ElideLeft
            anchors.fill: parent
            font.pixelSize: 12
            verticalAlignment: Text.AlignVCenter
            clip: true
            leftPadding: 5
            anchors.rightMargin: 35
        }
    }

    Text {
        id: projectPathLabel
        x: 10
        y: 178
        width: 177
        height: 15
        color: Constants.currentBrand
        text: prettyFilePath
        elide: Text.ElideLeft
        font.pixelSize: 12
        leftPadding: 5
        renderType: Text.NativeRendering
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
    }

    /*
    WelcomeScreen {
        id: welcomeScreen
    }*/

    Rectangle {
        id: tagBackground
        x: 10
        y: 199
        width: 84
        height: 15
        color: Constants.currentBrand

        Text {
            id: tagLabel
            color: Constants.brandGlobalText
            text: qsTr("tag name")
            anchors.fill: parent
            font.pixelSize: 10
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            font.weight: Font.ExtraLight
        }
    }
    states: [
        State {
            name: "normal"
            when: !mouseArea.containsMouse && !mouseArea.pressed

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
            }
        },
        State {
            name: "hover"
            when: mouseArea.containsMouse && !mouseArea.pressed

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
        },
        State {
            name: "press"
            when: mouseArea.containsMouse && mouseArea.pressed

            PropertyChanges {
                target: recentThumbBackground
                color: Constants.currentHoverThumbnailBackground
                border.color: Constants.currentBrand
                border.width: 2
            }

            PropertyChanges {
                target: projectNameBackground
                color: Constants.currentHoverThumbnailLabelBackground
                border.color: "#00000000"
            }
        }
    ]
}

/*##^##
Designer {
    D{i:0;formeditorZoom:0.5;height:760;width:1460}
}
##^##*/
