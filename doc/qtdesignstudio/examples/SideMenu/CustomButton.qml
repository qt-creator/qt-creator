/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Design Studio.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

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
