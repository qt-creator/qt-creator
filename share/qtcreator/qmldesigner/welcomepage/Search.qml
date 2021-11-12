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
import StudioTheme 1.0 as StudioTheme

Rectangle {
    id: searchBackground
    width: 622
    height: 25
    color: Constants.currentThumbnailGridBackground
    border.color: "#00000000"
    state: "normal"

    Text {
        id: searchLabel
        x: 28
        y: 0
        width: 595
        height: 25
        color: Constants.currentGlobalText
        text: qsTr("Search")
        font.pixelSize: 12
        horizontalAlignment: Text.AlignLeft
        verticalAlignment: Text.AlignVCenter
    }

    Text {
        id: glassPlaceholder
        width: 25
        opacity: 1
        color: Constants.currentGlobalText
        font.family: StudioTheme.Constants.iconFont.family
        text: StudioTheme.Constants.search
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        font.pixelSize: 14
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        anchors.leftMargin: 0
        anchors.topMargin: 0
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
    }

    Rectangle {
        id: searchResults
        y: 25
        height: 124
        color: Constants.currentNormalThumbnailBackground
        border.color: "#00000000"
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.rightMargin: 0
        anchors.leftMargin: 0

        Rectangle {
            id: searchHighlight
            y: 37
            height: 25
            color: Constants.currentNormalThumbnailLabelBackground
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.rightMargin: 0
            anchors.leftMargin: 0
        }

        Text {
            id: searchLabel1
            x: 28
            y: 6
            width: 595
            height: 25
            color: Constants.currentGlobalText
            text: qsTr("Result 1")
            font.pixelSize: 12
            horizontalAlignment: Text.AlignLeft
            verticalAlignment: Text.AlignVCenter
        }



        Text {
            id: searchLabel2
            y: 37
            height: 25
            color: Constants.currentBrand
            text: qsTr("Result2")
            anchors.left: parent.left
            anchors.right: parent.right
            font.pixelSize: 12
            horizontalAlignment: Text.AlignLeft
            verticalAlignment: Text.AlignVCenter
            anchors.rightMargin: 8
            anchors.leftMargin: 28
        }


        Text {
            id: searchLabel3
            x: 28
            y: 68
            width: 595
            height: 25
            color: Constants.currentGlobalText
            text: qsTr("Result3")
            font.pixelSize: 12
            horizontalAlignment: Text.AlignLeft
            verticalAlignment: Text.AlignVCenter
        }

    }
    states: [
        State {
            name: "normal"
            when: !mouseArea.containsMouse && !mouseArea.pressed

            PropertyChanges {
                target: searchLabel
                opacity: 0.69
            }

            PropertyChanges {
                target: glassPlaceholder
                opacity: 0.496
            }

            PropertyChanges {
                target: searchResults
                visible: false
            }

            PropertyChanges {
                target: searchBackground
                border.color: "#002e769e"
            }
        },
        State {
            name: "hover"
            when: mouseArea.containsMouse && !mouseArea.pressed

            PropertyChanges {
                target: searchResults
                visible: false
            }

            PropertyChanges {
                target: searchBackground
                border.color: "#002e769e"
            }
        },
        State {
            name: "checked"
            when: mouseArea.pressed

            PropertyChanges {
                target: searchResults
                visible: true
            }

            PropertyChanges {
                target: searchBackground
                border.color: Constants.currentBrand
            }

            PropertyChanges {
                target: searchHighlight
                color: Constants.currentBrand
            }

            PropertyChanges {
                target: searchLabel2
                color: Constants.currentActiveGlobalText
            }
        }
    ]
}

/*##^##
Designer {
    D{i:0;height:25;width:500}D{i:5}D{i:7}D{i:4}
}
##^##*/
