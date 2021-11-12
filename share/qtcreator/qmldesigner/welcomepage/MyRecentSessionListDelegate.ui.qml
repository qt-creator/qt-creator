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
import StudioTheme 1.0 as StudioTheme

Item {
    id: recentSessionListDelegate
    width: 475
    height: 58
    state: "collapsedNormal"
    property bool toggled: !expandToggle.collapsed

    Rectangle {
        id: actionsMenu
        x: 0
        y: 120
        width: 194
        height: 24
        color: Constants.currentNormalThumbnailLabelBackground
        border.color: "#00000000"

        Text {
            id: div1
            width: 5
            height: 15
            color: Constants.currentGlobalText
            text: qsTr("|")
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: cloneButton.right
            font.pixelSize: 12
            verticalAlignment: Text.AlignVCenter
            anchors.verticalCenterOffset: -1
        }

        Text {
            id: div2
            width: 5
            height: 15
            color: Constants.currentGlobalText
            text: qsTr("|")
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: renameButton.right
            font.pixelSize: 12
            verticalAlignment: Text.AlignVCenter
            anchors.leftMargin: 5
            anchors.verticalCenterOffset: -1
        }

        TextButton {
            id: cloneButton
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            anchors.leftMargin: 27
        }

        TextButton {
            id: renameButton
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: div1.right
            anchors.leftMargin: 5
            textButtonLabelText: "Rename"
        }

        TextButton {
            id: deleteButton
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: div2.right
            textButtonLabelText: "Delete"
        }
    }

    Rectangle {
        id: listsAndPath
        y: 58
        height: 58
        color: Constants.currentNormalThumbnailBackground
        border.color: "#00000000"
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: 0

        Text {
            id: cmake
            y: 8
            color: Constants.currentGlobalText
            text: qsTr("CMakeLists")
            anchors.left: parent.left
            font.pixelSize: 12
            verticalAlignment: Text.AlignVCenter
            anchors.leftMargin: 29
        }

        Text {
            id: path
            y: 29
            color: Constants.currentBrand
            text: qsTr("C://Users/user/pathtolist/CmakeLists.txt")
            anchors.left: parent.left
            font.pixelSize: 12
            verticalAlignment: Text.AlignVCenter
            anchors.leftMargin: 29
        }
    }

    Rectangle {
        id: recentSessionListDelegateBackground
        y: 0
        height: 58
        color: Constants.currentNormalThumbnailLabelBackground
        border.color: "#00000000"
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: 0

        Text {
            id: sessionNumber
            color: Constants.currentGlobalText
            text: qsTr("1.")
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            font.pixelSize: 12
            verticalAlignment: Text.AlignVCenter
            anchors.leftMargin: 10
        }

        Text {
            id: sessionName
            color: Constants.currentGlobalText
            text: qsTr("Default Session")
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: sessionNumber.right
            font.pixelSize: 12
            verticalAlignment: Text.AlignVCenter
            anchors.leftMargin: 10
            anchors.verticalCenterOffset: 0
        }
    }

    MouseArea {
        id: mouseAreaPressed
        anchors.fill: parent
        hoverEnabled: true
    }

    states: [
        State {
            name: "collapsedNormal"
            when: !recentSessionListDelegate.toggled
                  && !mouseAreaPressed.containsMouse
                  && !mouseAreaPressed.pressed && !myFavoriteToggle.isHovered
                  && !expandToggle.isHovered

            PropertyChanges {
                target: actionsMenu
                visible: false
            }

            PropertyChanges {
                target: listsAndPath
                visible: false
            }

            PropertyChanges {
                target: recentSessionListDelegateBackground
                visible: true
                border.color: "#002f779f"
            }
        },
        State {
            name: "expandedNormal"
            when: recentSessionListDelegate.toggled
                  && !mouseAreaPressed.containsMouse
                  && !mouseAreaPressed.pressed && !myFavoriteToggle.isHovered
                  && !expandToggle.isHovered

            PropertyChanges {
                target: recentSessionListDelegateBackground
                visible: true
                border.color: "#002f779f"
            }

            PropertyChanges {
                target: listsAndPath
                visible: true
            }

            PropertyChanges {
                target: actionsMenu
                visible: true
            }

            PropertyChanges {
                target: recentSessionListDelegate
                height: 144
            }

            PropertyChanges {
                target: myFavoriteToggle
                anchors.verticalCenterOffset: -43
            }

            PropertyChanges {
                target: mouseAreaPressed
                anchors.bottomMargin: 86
            }
        },
        State {
            name: "collapsedHover"
            when: !recentSessionListDelegate.toggled
                  && ((mouseAreaPressed.containsMouse
                       || myFavoriteToggle.isHovered || expandToggle.isHovered)
                      && !mouseAreaPressed.pressed)
            PropertyChanges {
                target: actionsMenu
                visible: false
            }

            PropertyChanges {
                target: listsAndPath
                visible: false
            }

            PropertyChanges {
                target: recentSessionListDelegateBackground
                visible: true
                color: Constants.currentHoverThumbnailLabelBackground
                border.color: "#002f779f"
            }
        },
        State {
            name: "expandedHover"
            when: recentSessionListDelegate.toggled
                  && ((mouseAreaPressed.containsMouse
                       || myFavoriteToggle.isHovered || expandToggle.isHovered)
                      && !mouseAreaPressed.pressed)
            PropertyChanges {
                target: recentSessionListDelegateBackground
                visible: true
                color: Constants.currentHoverThumbnailLabelBackground
                border.color: "#002f779f"
            }

            PropertyChanges {
                target: listsAndPath
                visible: true
            }

            PropertyChanges {
                target: actionsMenu
                visible: true
            }

            PropertyChanges {
                target: recentSessionListDelegate
                height: 144
            }

            PropertyChanges {
                target: myFavoriteToggle
                anchors.verticalCenterOffset: -43
            }

            PropertyChanges {
                target: mouseAreaPressed
                anchors.bottomMargin: 86
            }
        },
        State {
            name: "collapsedPressed"
            when: !recentSessionListDelegate.toggled
                  && ((mouseAreaPressed.containsMouse
                       || myFavoriteToggle.isHovered || expandToggle.isHovered)
                      && mouseAreaPressed.pressed)
            PropertyChanges {
                target: actionsMenu
                visible: false
            }

            PropertyChanges {
                target: listsAndPath
                visible: false
            }

            PropertyChanges {
                target: recentSessionListDelegateBackground
                visible: true
                color: Constants.currentHoverThumbnailLabelBackground
                border.color: Constants.currentBrand
                border.width: 2
            }
        },
        State {
            name: "expandedPressed"
            when: recentSessionListDelegate.toggled
                  && ((mouseAreaPressed.containsMouse
                       || myFavoriteToggle.isHovered || expandToggle.isHovered)
                      && mouseAreaPressed.pressed)
            PropertyChanges {
                target: recentSessionListDelegateBackground
                visible: true
                color: Constants.currentHoverThumbnailLabelBackground
                border.color: Constants.currentBrand
                border.width: 2
            }

            PropertyChanges {
                target: listsAndPath
                visible: true
            }

            PropertyChanges {
                target: actionsMenu
                visible: true
            }

            PropertyChanges {
                target: expandSessionButton
                rotation: 180
            }

            PropertyChanges {
                target: recentSessionListDelegate
                height: 144
            }

            PropertyChanges {
                target: myFavoriteToggle
                anchors.verticalCenterOffset: -41
            }

            PropertyChanges {
                target: mouseAreaPressed
                anchors.bottomMargin: 86
            }
        }
    ]

    FavoriteToggle {
        id: myFavoriteToggle
        x: 411
        y: 14
        height: 30
        anchors.right: expandToggle.left
    }

    ExpandToggle {
        id: expandToggle
        x: 439
        y: 16
        width: 36
        height: 30
        anchors.right: recentSessionListDelegateBackground.right
    }
}

/*##^##
Designer {
    D{i:0;height:58;width:455}D{i:7}D{i:10}
}
##^##*/

