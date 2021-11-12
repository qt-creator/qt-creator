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

import QtQuick 2.15
import WelcomeScreen 1.0

Item {
    id: recentProjectListDelegate
    width: 700
    height: 58
    state: "normal"

    Rectangle {
        id: listItemBackground
        color: Constants.currentNormalThumbnailBackground
        border.color: "#00000000"
        anchors.fill: parent
    }

    Rectangle {
        id: listItemSquare
        x: 142
        width: 58
        color: Constants.currentNormalThumbnailLabelBackground
        border.color: "#00000000"
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.rightMargin: 2
        anchors.bottomMargin: 2
        anchors.topMargin: 2
    }

    Rectangle {
        id: listItemTitleBack
        height: 25
        color: Constants.currentNormalThumbnailLabelBackground
        border.color: "#00000000"
        anchors.left: parent.left
        anchors.right: listItemSquare.left
        anchors.top: parent.top
        anchors.leftMargin: 2
        anchors.topMargin: 2
        anchors.rightMargin: 0
    }

    Text {
        id: projectNameLabel
        x: 57
        y: 0
        width: 585
        height: 25
        color: Constants.currentGlobalText
        text: qsTr("My Project Name")
        font.pixelSize: 12
        horizontalAlignment: Text.AlignLeft
        verticalAlignment: Text.AlignVCenter
    }

    Text {
        id: projectTypeLabel
        x: 9
        y: 0
        width: 23
        height: 25
        color: Constants.currentGlobalText
        text: qsTr("C++")
        font.pixelSize: 12
        verticalAlignment: Text.AlignVCenter
    }

    Text {
        id: projectPathLabel
        x: 57
        y: 25
        width: 585
        height: 33
        color: Constants.currentBrand
        text: qsTr("path/to/my/project")
        font.pixelSize: 12
        verticalAlignment: Text.AlignVCenter
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        propagateComposedEvents: true
        hoverEnabled: true
    }
    states: [
        State {
            name: "normal"
            when: !mouseArea.containsMouse && !mouseArea.pressed
                  && !myFavoriteToggle.isHovered

            PropertyChanges {
                target: listItemBackground
                border.color: "#002f779f"
            }
        },
        State {
            name: "hover"
            when: ((mouseArea.containsMouse || myFavoriteToggle.isHovered)
                   && !mouseArea.pressed)

            PropertyChanges {
                target: listItemTitleBack
                color: Constants.currentHoverThumbnailLabelBackground
            }

            PropertyChanges {
                target: listItemSquare
                color: Constants.currentHoverThumbnailLabelBackground
            }

            PropertyChanges {
                target: listItemBackground
                color: Constants.currentHoverThumbnailBackground
                border.color: "#002f779f"
            }
        },
        State {
            name: "pressed"
            when: mouseArea.containsMouse && mouseArea.pressed
            PropertyChanges {
                target: listItemTitleBack
                color: Constants.currentHoverThumbnailLabelBackground
            }

            PropertyChanges {
                target: listItemSquare
                color: Constants.currentHoverThumbnailLabelBackground
            }

            PropertyChanges {
                target: listItemBackground
                color: Constants.currentHoverThumbnailBackground
                border.color: Constants.currentBrand
                border.width: 2
            }
        }
    ]
    FavoriteToggle {
        id: myFavoriteToggle
        x: 658
        y: 14
        anchors.right: parent.right
        anchors.rightMargin: 13
    }
}

/*##^##
Designer {
    D{i:0;height:58;width:957}
}
##^##*/

