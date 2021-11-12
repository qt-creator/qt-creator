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

Rectangle {
    id: listGrid
    width: 61
    height: 25
    color: Constants.currentNormalThumbnailBackground
    border.color: "#00000000"
    property bool viewDefault: true
    state: "grid"

    Rectangle {
        id: divider
        width: 1
        height: 21
        color: "#ffffff"
        anchors.verticalCenter: parent.verticalCenter
        anchors.horizontalCenter: parent.horizontalCenter
    }

    Item {
        id: listIcon
        x: 40
        y: 5

        Rectangle {
            id: l1
            x: 0
            y: 0
            width: 14
            height: 2
            color: "#ffffff"
        }

        Rectangle {
            id: l2
            x: 0
            y: 4
            width: 14
            height: 2
            color: "#ffffff"
        }

        Rectangle {
            id: l3
            x: 0
            y: 8
            width: 14
            height: 2
            color: "#ffffff"
        }

        Rectangle {
            id: l4
            x: 0
            y: 12
            width: 14
            height: 2
            color: "#ffffff"
        }
    }

    Item {
        id: gridIcon
        x: 7
        y: 5
        Rectangle {
            id: l5
            x: 0
            y: 0
            width: 6
            height: 6
            color: "#ffffff"
        }

        Rectangle {
            id: l6
            x: 8
            y: 0
            width: 6
            height: 6
            color: "#ffffff"
        }

        Rectangle {
            id: l7
            x: 8
            y: 8
            width: 6
            height: 6
            color: "#ffffff"
        }

        Rectangle {
            id: l8
            x: 0
            y: 8
            width: 6
            height: 6
            color: "#ffffff"
        }
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true

        Connections {
            target: mouseArea
            onClicked: listGrid.viewDefault = !listGrid.viewDefault
        }
    }
    states: [
        State {
            name: "grid"
            when: listGrid.viewDefault && !mouseArea.containsMouse

            PropertyChanges {
                target: l5
                color: Constants.currentBrand
                border.color: "#00000000"
            }

            PropertyChanges {
                target: l6
                color: Constants.currentBrand
                border.color: "#00000000"
            }

            PropertyChanges {
                target: l8
                color: Constants.currentBrand
                border.color: "#00000000"
            }

            PropertyChanges {
                target: l7
                color: Constants.currentBrand
                border.color: "#00000000"
            }

            PropertyChanges {
                target: divider
                color: Constants.currentGlobalText
            }

            PropertyChanges {
                target: l1
                color: Constants.currentGlobalText
                border.color: "#00000000"
            }

            PropertyChanges {
                target: l2
                color: Constants.currentGlobalText
                border.color: "#00000000"
            }

            PropertyChanges {
                target: l3
                color: Constants.currentGlobalText
                border.color: "#00000000"
            }

            PropertyChanges {
                target: l4
                color: Constants.currentGlobalText
                border.color: "#00000000"
            }
        },
        State {
            name: "list"
            when: !listGrid.viewDefault && !mouseArea.containsMouse

            PropertyChanges {
                target: l5
                color: Constants.currentGlobalText
                border.color: "#00000000"
            }

            PropertyChanges {
                target: l8
                color: Constants.currentGlobalText
                border.color: "#00000000"
            }

            PropertyChanges {
                target: l6
                color: Constants.currentGlobalText
                border.color: "#00000000"
            }

            PropertyChanges {
                target: l7
                color: Constants.currentGlobalText
                border.color: "#00000000"
            }

            PropertyChanges {
                target: l1
                color: Constants.currentBrand
                border.color: "#00000000"
            }

            PropertyChanges {
                target: l2
                color: Constants.currentBrand
                border.color: "#00000000"
            }

            PropertyChanges {
                target: l3
                color: Constants.currentBrand
                border.color: "#00000000"
            }

            PropertyChanges {
                target: l4
                color: Constants.currentBrand
                border.color: "#00000000"
            }

            PropertyChanges {
                target: listGrid
                color: Constants.currentNormalThumbnailBackground
            }
        },
        State {
            name: "gridHover"
            when: mouseArea.containsMouse && listGrid.viewDefault
            PropertyChanges {
                target: l5
                color: Constants.currentBrand
                border.color: "#00000000"
            }

            PropertyChanges {
                target: l6
                color: Constants.currentBrand
                border.color: "#00000000"
            }

            PropertyChanges {
                target: l8
                color: Constants.currentBrand
                border.color: "#00000000"
            }

            PropertyChanges {
                target: l7
                color: Constants.currentBrand
                border.color: "#00000000"
            }

            PropertyChanges {
                target: divider
                color: Constants.currentGlobalText
            }

            PropertyChanges {
                target: l1
                color: Constants.currentGlobalText
                border.color: "#00000000"
            }

            PropertyChanges {
                target: l2
                color: Constants.currentGlobalText
                border.color: "#00000000"
            }

            PropertyChanges {
                target: l3
                color: Constants.currentGlobalText
                border.color: "#00000000"
            }

            PropertyChanges {
                target: l4
                color: Constants.currentGlobalText
                border.color: "#00000000"
            }

            PropertyChanges {
                target: listGrid
                color: Constants.currentHoverThumbnailBackground
            }
        },
        State {
            name: "listHover"
            when: mouseArea.containsMouse && !listGrid.viewDefault
            PropertyChanges {
                target: l5
                color: Constants.currentGlobalText
                border.color: "#00000000"
            }

            PropertyChanges {
                target: l8
                color: Constants.currentGlobalText
                border.color: "#00000000"
            }

            PropertyChanges {
                target: l6
                color: Constants.currentGlobalText
                border.color: "#00000000"
            }

            PropertyChanges {
                target: l7
                color: Constants.currentGlobalText
                border.color: "#00000000"
            }

            PropertyChanges {
                target: l1
                color: Constants.currentBrand
                border.color: "#00000000"
            }

            PropertyChanges {
                target: l2
                color: Constants.currentBrand
                border.color: "#00000000"
            }

            PropertyChanges {
                target: l3
                color: Constants.currentBrand
                border.color: "#00000000"
            }

            PropertyChanges {
                target: l4
                color: Constants.currentBrand
                border.color: "#00000000"
            }

            PropertyChanges {
                target: listGrid
                color: Constants.currentHoverThumbnailBackground
            }
        }
    ]
}

/*##^##
Designer {
    D{i:0;height:25;width:61}
}
##^##*/
