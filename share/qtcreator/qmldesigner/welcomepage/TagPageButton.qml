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
    id: tagPageButton
    width: 13
    height: 15
    state: "normal"

    Text {
        id: tagPageIcon
        x: 2
        y: -1
        width: 8
        height: 16
        color: Constants.currentBrand
        text: qsTr("<")
        font.pixelSize: 13
        Layout.leftMargin: 3
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
    }
    states: [
        State {
            name: "normal"
            when: mouseArea.pressed
        },
        State {
            name: "pressed"
            when: mouseArea.pressed && mouseArea.containsMouse

            PropertyChanges {
                target: tagPageIcon
                scale: 1.1
            }
        },
        State {
            name: "hovered"
            when: mouseArea.containsMouse && !mouseArea.pressed
            PropertyChanges {
                target: tagPageIcon
                scale: 1.2
            }

            PropertyChanges {
                target: mouseArea
                hoverEnabled: true
            }
        }
    ]
}

/*##^##
Designer {
    D{i:0;formeditorZoom:16;height:15;width:13}D{i:2}
}
##^##*/
