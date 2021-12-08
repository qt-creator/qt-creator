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
import QtQuick.Templates 2.5
import WelcomeScreen 1.0

Button {
    id: control

    implicitWidth: Math.max(
                       buttonBackground ? buttonBackground.implicitWidth : 0,
                       textItem.implicitWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(
                        buttonBackground ? buttonBackground.implicitHeight : 0,
                        textItem.implicitHeight + topPadding + bottomPadding)
    leftPadding: 4
    rightPadding: 4

    text: "My Button"
    checkable: true

    property bool decorated: false
    state: "normal"

    background: buttonBackground
    Rectangle {
        id: buttonBackground
        color: "#00000000"
        implicitWidth: 100
        implicitHeight: 40
        opacity: enabled ? 1 : 0.3
        radius: 2
        border.color: "#047eff"
        anchors.fill: parent
    }

    contentItem: textItem
    Rectangle {
        id: decoration
        width: 10
        visible: control.decorated
        color: Constants.currentBrand
        border.color: Constants.currentBrand
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.rightMargin: 1
        anchors.bottomMargin: 1
        anchors.topMargin: 1
    }

    Text {
        id: textItem
        text: control.text
        font.pixelSize: 18

        opacity: enabled ? 1.0 : 0.3
        color: "#047eff"
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }

    states: [
        State {
            name: "normal"
            when: !control.down && !control.hovered && !control.checked

            PropertyChanges {
                target: buttonBackground
                color: Constants.currentPushButtonNormalBackground
                border.color: Constants.currentPushButtonNormalOutline
            }

            PropertyChanges {
                target: textItem
                color: Constants.currentGlobalText
            }
        },
        State {
            name: "hover"
            when: control.hovered && !control.checked && !control.down
            PropertyChanges {
                target: textItem
                color: Constants.currentGlobalText
            }

            PropertyChanges {
                target: buttonBackground
                color: Constants.currentPushButtonHoverBackground
                border.color: Constants.currentPushButtonHoverOutline
            }
        },
        State {
            name: "activeQds"
            when: Constants.defaultBrand && (control.checked || control.down)
            PropertyChanges {
                target: textItem
                color: Constants.currentActiveGlobalText
            }

            PropertyChanges {
                target: buttonBackground
                color: Constants.currentBrand
                border.color: "#00000000"
            }
        },
        State {
            name: "activeCreator"
            when: !Constants.defaultBrand && (control.checked || control.down)
            PropertyChanges {
                target: textItem
                color: Constants.currentActiveGlobalText
            }

            PropertyChanges {
                target: buttonBackground
                color: Constants.currentBrand
                border.color: "#00000000"
            }
        }
    ]
}

/*##^##
Designer {
    D{i:0;height:40;width:266}
}
##^##*/

