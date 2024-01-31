// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Templates
import QtQuick.Layouts
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

    text: ""
    visible: true
    state: "normal"

    property alias fontpixelSize: textItem.font.pixelSize
    property bool decorated: false

    background: Rectangle {
        id: buttonBackground
        color: "#fca4a4"
        implicitWidth: 100
        implicitHeight: 40
        opacity: buttonBackground.enabled ? 1 : 0.3
        radius: 2
        border.color: "#047eff"
        anchors.fill: parent
    }

    contentItem: Text {
        id: textItem
        visible: false
        text: qsTr("Text")
        font.pixelSize: 12
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

    RowLayout {
        anchors.fill: parent

        Item {
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
            width: 200
            height: 200
            Layout.fillHeight: true
            Layout.fillWidth: true
        }

        FigmaButton {
            id: figmaButton
            Layout.maximumHeight: 15
            Layout.minimumHeight: 15
            Layout.preferredHeight: 15
            Layout.fillHeight: false
            Layout.preferredWidth: 25
        }

        Item {
            width: 200
            height: 200
            Layout.fillHeight: true
            Layout.fillWidth: true
        }
    }

    states: [
        State {
            name: "normal"
            when: !youtubeButton.isHovered && !figmaButton.isHovered && !control.hovered

            PropertyChanges {
                target: buttonBackground
                color: Constants.currentPushButtonNormalBackground
                border.color: Constants.currentPushButtonNormalOutline
            }
        },
        State {
            name: "hover"
            when: control.hovered || youtubeButton.isHovered || figmaButton.isHovered

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

