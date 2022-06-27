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
import QtQuick.Templates 2.15
import LandingPage as Theme

Button {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)
    leftPadding: 4
    rightPadding: 4
    hoverEnabled: true
    font.family: Theme.Values.baseFont
    font.pixelSize: 16

    background: Rectangle {
        id: buttonBackground
        color: Theme.Colors.backgroundPrimary
        implicitWidth: 100
        implicitHeight: 35
        border.color: Theme.Colors.foregroundSecondary
        anchors.fill: parent
    }

    contentItem: Text {
        id: textItem
        text: control.text
        font: control.font
        color: Theme.Colors.text
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        rightPadding: 5
        leftPadding: 5
    }

    states: [
        State {
            name: "default"
            when: control.enabled && !control.hovered && !control.pressed && !control.checked
            PropertyChanges {
                target: buttonBackground
                color: Theme.Colors.backgroundPrimary
            }
            PropertyChanges {
                target: textItem
                color: Theme.Colors.text
            }
        },
        State {
            name: "hover"
            extend: "default"
            when: control.enabled && control.hovered && !control.pressed
            PropertyChanges {
                target: buttonBackground
                color: Theme.Colors.hover
            }
        },
        State {
            name: "press"
            extend: "default"
            when: control.hovered && control.pressed
            PropertyChanges {
                target: buttonBackground
                color: Theme.Colors.accent
                border.color: Theme.Colors.accent
            }
            PropertyChanges {
                target: textItem
                color: Theme.Colors.backgroundPrimary
            }
        },
        State {
            name: "disable"
            when: !control.enabled
            PropertyChanges {
                target: buttonBackground
                color: Theme.Colors.backgroundPrimary
                border.color: Theme.Colors.disabledLink
            }
            PropertyChanges {
                target: textItem
                color: Theme.Colors.disabledLink
            }
        }
    ]
}

/*##^##
Designer {
    D{i:0;height:40;width:142}
}
##^##*/

