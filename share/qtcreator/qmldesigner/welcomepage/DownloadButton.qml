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

Item {
    id: favoriteToggle
    width: 30
    height: 30
    property bool isHovered: false
    state: "normal"

    Text {
        id: downloadableIcon
        color: "#de7de1"
        font.family: StudioTheme.Constants.iconFont.family
        text: StudioTheme.Constants.download
        anchors.fill: parent
        font.pixelSize: 22
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        anchors.bottomMargin: 0
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        propagateComposedEvents: true
    }
    states: [
        State {
            name: "normal"
            when: !mouseArea.pressed && !mouseArea.containsMouse

            PropertyChanges {
                target: downloadableIcon
                color: Constants.currentGlobalText
            }
        },
        State {
            name: "pressed"
            when: mouseArea.pressed && mouseArea.containsMouse

            PropertyChanges {
                target: downloadableIcon
                color: Constants.currentBrand
                scale: 1.2
            }
            PropertyChanges {
                target: favoriteToggle
                isHovered: true
            }
        },
        State {
            name: "hover"
            when: mouseArea.containsMouse && !mouseArea.pressed
            PropertyChanges {
                target: downloadableIcon
                color: Constants.currentGlobalText
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
    D{i:0;formeditorZoom:1.33;height:30;width:30}
}
##^##*/
