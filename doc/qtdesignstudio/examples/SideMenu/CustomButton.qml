// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick 2.10
import QtQuick.Templates 2.1 as T
import SideMenu 1.0

T.Button {
    id: control
    width: 180
    height: 70

    font: Constants.font
    implicitWidth: Math.max(background ? background.implicitWidth : 0,
                                         contentItem.implicitWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(background ? background.implicitHeight : 0,
                                          contentItem.implicitHeight + topPadding + bottomPadding)
    leftPadding: 4
    rightPadding: 4

    text: control.state
    property alias text1Text: text1.text
    autoExclusive: false
    checkable: true


    Image {
        id: background
        x: 15
        y: 17
        source: "assets/inactive_button.png"
    }

    Image {
        id: contentItem
        x: -1
        y: 0
        visible: true
        source: "assets/hover_button.png"
    }

    Image {
        id: image2
        x: 0
        y: -1
        visible: true
        source: "assets/active_button.png"
    }

    Text {
        id: text1
        x: -1
        y: 16
        width: 163
        height: 16
        color: "#ffffff"
        text: "PRESSED"
        horizontalAlignment: Text.AlignHCenter
        font.weight: Font.ExtraLight
        font.family: "IBM Plex Sans"
        font.pixelSize: 12
    }

    states: [
        State {
            name: "checked"
            when: control.checked

            PropertyChanges {
                target: background
                x: 16
                y: 16
                width: 156
                visible: false
            }

            PropertyChanges {
                target: contentItem
                visible: false
            }

            PropertyChanges {
                target: image2
                x: 0
                y: 0
                visible: true
            }

            PropertyChanges {
                target: text1
                x: 0
                y: 16
                width: 162
                height: 16
                text: "CHECKED"
                horizontalAlignment: Text.AlignHCenter
            }

        },
        State {
            name: "hover"
            when: control.hovered && !control.checked && !control.pressed

            PropertyChanges {
                target: image2
                x: 0
                visible: false
            }

            PropertyChanges {
                target: background
                x: 16
                y: 16
                visible: false
            }

            PropertyChanges {
                target: contentItem
                visible: true
            }

            PropertyChanges {
                target: text1
                x: -1
                y: 16
                width: 162
                height: 16
                color: "#d07533"
                text: "HOVER"
                horizontalAlignment: Text.AlignHCenter
            }


        },
        State {
            name: "normal"
            when: !control.pressed && !control.checked &&!control.hovered


        PropertyChanges {
            target: image2
            visible: false
        }

        PropertyChanges {
            target: contentItem
            visible: false
        }

        PropertyChanges {
            target: background
            visible: true
        }

        PropertyChanges {
            target: text1
            x: 15
            y: 33
            width: 163
            height: 16
            color: "#d07533"
            text: "NORMAL"
            horizontalAlignment: Text.AlignHCenter
        }
        }
    ]
}
