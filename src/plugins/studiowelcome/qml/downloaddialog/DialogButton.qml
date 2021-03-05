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

import QtQuick 2.10
import QtQuick.Templates 2.1 as T

T.Button {
    id: control

    implicitWidth: Math.max(buttonBackground ? buttonBackground.implicitWidth : 0,
                                         textItem.implicitWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(buttonBackground ? buttonBackground.implicitHeight : 0,
                                          textItem.implicitHeight + topPadding + bottomPadding)
    leftPadding: 4
    rightPadding: 4

    text: "My Button"

    property color defaultColor: "#b9b9ba"
    property color checkedColor:  "#ffffff"

    background: buttonBackground
    Rectangle {
        id: buttonBackground
        color: control.defaultColor
        implicitWidth: 100
        implicitHeight: 40
        opacity: enabled ? 1 : 0.3
        radius: 0
        border.width: 1
    }

    contentItem: textItem
    Text {
        id: textItem
        text: control.text

        opacity: enabled ? 1.0 : 0.3
        color: "#bababa"
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }

    states: [
        State {
            name: "normal"
            when: !control.down && !control.checked
            PropertyChanges {
                target: buttonBackground
                color:  "#2d2e30"
            }
        },
        State {
            name: "down"
            when: control.down || control.checked
            PropertyChanges {
                target: textItem
                color: control.checkedColor
            }

            PropertyChanges {
                target: buttonBackground
                color: "#545456"
                border.color: "#70a2f5"
                border.width: 2
            }
        }
    ]
}
