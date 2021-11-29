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
    id: expandToggle
    width: 36
    height: 38
    property bool collapsed: true
    property bool isHovered: false

    Text {
        id: expandSessionButton
        y: 8
        color: "#efafaf"
        font.family: StudioTheme.Constants.iconFont.family
        text: StudioTheme.Constants.adsDropDown
        anchors.fill: parent
        font.pixelSize: 18
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }

    MouseArea {
        id: mouseAreaToggle
        anchors.fill: parent
        hoverEnabled: true

        Connections {
            target: mouseAreaToggle
            function onReleased(mouse) { expandToggle.collapsed = !expandToggle.collapsed }
        }
    }
    states: [
        State {
            name: "collapseNormal"
            when: expandToggle.collapsed && !mouseAreaToggle.containsMouse
                  && !mouseAreaToggle.pressed

            PropertyChanges {
                target: expandSessionButton
                color: Constants.currentGlobalText
            }
        },

        State {
            name: "collapseHover"
            when: expandToggle.collapsed && mouseAreaToggle.containsMouse
                  && !mouseAreaToggle.pressed
            PropertyChanges {
                target: expandSessionButton
                color: Constants.currentGlobalText
                scale: 1.1
            }
            PropertyChanges {
                target: expandToggle
                isHovered: true
            }
        },
        State {
            name: "CollapsePress"
            when: expandToggle.collapsed && mouseAreaToggle.containsMouse
                  && mouseAreaToggle.pressed

            PropertyChanges {
                target: expandSessionButton
                color: Constants.currentBrand
                scale: 1.2
            }
            PropertyChanges {
                target: expandToggle
                isHovered: true
            }
        },
        State {
            name: "expandNormal"
            when: !expandToggle.collapsed && !mouseAreaToggle.containsMouse
                  && !mouseAreaToggle.pressed

            PropertyChanges {
                target: expandSessionButton
                color: Constants.currentGlobalText
                rotation: 180
            }
        },
        State {
            name: "expandHover"
            when: !expandToggle.collapsed && mouseAreaToggle.containsMouse
                  && !mouseAreaToggle.pressed
            PropertyChanges {
                target: expandSessionButton
                color: Constants.currentGlobalText
                scale: 1.1
                rotation: 180
            }
            PropertyChanges {
                target: expandToggle
                isHovered: true
            }
        },
        State {
            name: "expandPress"
            when: !expandToggle.collapsed && mouseAreaToggle.containsMouse
                  && mouseAreaToggle.pressed
            PropertyChanges {
                target: expandSessionButton
                color: Constants.currentBrand
                scale: 1.2
                rotation: 180
            }
            PropertyChanges {
                target: expandToggle
                isHovered: true
            }
        }
    ]
}

/*##^##
Designer {
    D{i:0;height:30;width:36}D{i:1}
}
##^##*/

