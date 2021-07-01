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
import StudioTheme 1.0 as StudioTheme

Rectangle {
    id: root

    property Item myControl
    property string value

    width: StudioTheme.Values.height
    height: StudioTheme.Values.height

    border.width: StudioTheme.Values.border
    border.color: "transparent"

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        onClicked: myControl.originSelectorClicked(root.value)
    }

    states: [
        State {
            name: "default"
            when: !mouseArea.containsMouse && !mouseArea.pressed && myControl.origin !== root.value
            PropertyChanges {
                target: root
                color: StudioTheme.Values.themeTextColorDisabled // TODO
            }
        },
        State {
            name: "hover"
            when: mouseArea.containsMouse && !mouseArea.pressed && myControl.origin !== root.value
            PropertyChanges {
                target: root
                color: StudioTheme.Values.themeControlBackgroundInteraction // TODO
            }
        },
        State {
            name: "press"
            when: mouseArea.containsPress && myControl.origin !== root.value
            PropertyChanges {
                target: root
                color: StudioTheme.Values.themeControlBackgroundInteraction // TODO
                border.color: StudioTheme.Values.themeInteraction
            }
        },
        State {
            name: "active"
            when: myControl.origin === root.value
            PropertyChanges {
                target: root
                color: StudioTheme.Values.themeInteraction
            }
        }
    ]
}
