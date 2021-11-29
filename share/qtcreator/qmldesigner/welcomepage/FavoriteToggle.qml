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
import StudioTheme 1.0 as StudioTheme


Item {
    id: favoriteToggle
    width: 30
    height: 30
    property bool isFavorite: false
    property bool isHovered: false
    state: "normal"

    Text {
        id: star
        color: Constants.currentGlobalText
        font.family: StudioTheme.Constants.iconFont.family
        text: StudioTheme.Constants.favorite
        anchors.fill: parent
        font.pixelSize: 18
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        anchors.topMargin: 0
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        propagateComposedEvents: true

        Connections {
            target: mouseArea
            function onReleased(mouse) { favoriteToggle.isFavorite = !favoriteToggle.isFavorite }
        }
    }
    states: [
        State {
            name: "normal"
            when: !favoriteToggle.isFavorite && !mouseArea.containsMouse && !mouseArea.pressed
        },
        State {
            name: "favorite"
            when: favoriteToggle.isFavorite && !mouseArea.containsMouse && !mouseArea.pressed

            PropertyChanges {
                target: star
                color: "#eee626"
            }
        },
        State {
            name: "hoverNotFavorite"
            when: mouseArea.containsMouse && !mouseArea.pressed && !favoriteToggle.isFavorite
            PropertyChanges {
                target: star
                scale: 1.1
            }
            PropertyChanges {
                target: favoriteToggle
                isHovered: true
            }
        },
        State {
            name: "hoverFavorite"
            when: mouseArea.containsMouse && !mouseArea.pressed && favoriteToggle.isFavorite
            PropertyChanges {
                target: star
                color: "#eee626"
                scale: 1.1
            }
            PropertyChanges {
                target: favoriteToggle
                isHovered: true
            }
        },
        State {
            name: "pressedNotFavorite"
            when: mouseArea.containsMouse && mouseArea.pressed && !favoriteToggle.isFavorite
            PropertyChanges {
                target: star
                scale: 1.2
            }
            PropertyChanges {
                target: favoriteToggle
                isHovered: true
            }
        },
        State {
            name: "pressedFavorite"
            when: mouseArea.containsMouse && mouseArea.pressed && favoriteToggle.isFavorite
            PropertyChanges {
                target: star
                color: "#eee626"
                font.pixelSize: 18
                scale: 1.2
            }
            PropertyChanges {
                target: favoriteToggle
                isHovered: true
            }
        }
    ]
}

/*##^##
Designer {
    D{i:0;height:30;width:30}
}
##^##*/
