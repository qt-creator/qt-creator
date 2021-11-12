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
import QtQuick.Controls 2.12
import WelcomeScreen 1.0
import QtQuick.Layouts 1.0

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

    text: ""
    property alias fontpixelSize: textItem.font.pixelSize
    property bool decorated: false
    visible: true
    state: "normal"

    background: buttonBackground
    Rectangle {
        id: buttonBackground
        color: "#fca4a4"
        implicitWidth: 100
        implicitHeight: 40
        opacity: enabled ? 1 : 0.3
        radius: 2
        border.color: "#047eff"
        anchors.fill: parent
    }

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
        visible: false
        text: qsTr("Text")
        font.pixelSize: 12
    }

    RowLayout {
        anchors.fill: parent

        Item {
            id: spacer
            width: 200
            height: 200
            Layout.fillWidth: true
            Layout.fillHeight: true
        }

        YoutubeButton {
            id: youtubeButton
            Layout.fillHeight: true
            Layout.preferredWidth: 60
        }

        Item {
            id: spacer1
            width: 200
            height: 200
            Layout.fillHeight: true
            Layout.fillWidth: true
        }

        TwitterButton {
            id: twitterButton
            Layout.maximumHeight: 15
            Layout.minimumHeight: 15
            Layout.preferredHeight: 15
            Layout.fillHeight: false
            Layout.preferredWidth: 25

        }

        Item {
            id: spacer2
            width: 200
            height: 200
            Layout.fillHeight: true
            Layout.fillWidth: true
        }
    }

    states: [
        State {
            name: "normal"
            when: !youtubeButton.isHovered && !twitterButton.isHovered
                  && !control.hovered

            PropertyChanges {
                target: buttonBackground
                color: Constants.currentPushButtonNormalBackground
                border.color: Constants.currentPushButtonNormalOutline
            }
        },
        State {
            name: "hover"
            when: control.hovered || youtubeButton.isHovered
                  || twitterButton.isHovered

            PropertyChanges {
                target: buttonBackground
                color: Constants.currentPushButtonHoverBackground
                border.color: Constants.currentPushButtonHoverOutline
            }
        }
    ]
}

/*##^##
Designer {
    D{i:0;formeditorZoom:4;height:25;width:207}D{i:3;locked:true}D{i:7}D{i:9}
}
##^##*/

