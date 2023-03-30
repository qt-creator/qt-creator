/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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
import StudioTheme 1.0 as StudioTheme
import StudioControls 1.0 as StudioControls

Item {
    id: root
    width: 25
    height: 25

    property bool hovered: mouseArea.containsMouse
    property bool checked: false
    signal pressed()

    Rectangle {
        id: background
        color: "transparent"
        anchors.fill: parent
    }

    // Burger menu icon
    Column {
        id: menuIcon
        anchors.verticalCenter: parent.verticalCenter
        anchors.horizontalCenter: parent.horizontalCenter
        spacing: 3

        property color iconColor: StudioTheme.Values.themeTextColor

        Label {
            id: moreMenu
            anchors.fill: parent
            text: StudioTheme.Constants.more_medium
            color: StudioTheme.Values.themeTextColor
            font.family: StudioTheme.Constants.iconFont.family
            font.pixelSize: 16 //StudioTheme.Values.myIconFontSize
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignHCenter
        }
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        onPressed: root.pressed()
    }

    states: [
        State {
            name: "default"
            when: !root.hovered && !root.checked

            PropertyChanges {
                target: background
                color: "transparent"
            }
            PropertyChanges {
                target: menuIcon
                iconColor: StudioTheme.Values.themeTextColor
            }
        },
        State {
            name: "hover"
            when: root.hovered && !root.checked

            PropertyChanges {
                target: background
                color: StudioTheme.Values.themeControlBackgroundHover
            }
            PropertyChanges {
                target: menuIcon
                iconColor: StudioTheme.Values.themeTextColor
            }
        },
        State {
            name: "checked"
            when: !root.hovered && root.checked

            PropertyChanges {
                target: menuIcon
                iconColor: StudioTheme.Values.themeInteraction
            }
        },
        State {
            name: "hoverChecked"
            when: root.hovered && root.checked

            PropertyChanges {
                target: background
                color: StudioTheme.Values.themeControlBackground_topToolbarHover
            }
            PropertyChanges {
                target: menuIcon
                iconColor: StudioTheme.Values.themeInteraction
            }
        }
    ]
}
