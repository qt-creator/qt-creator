
/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
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
import QtQuick 2.12
import loginui4 1.0
import QtQuick.Controls 2.15
import QtQuick.Timeline 1.0

Rectangle {
    id: rectangle
    width: Constants.width
    height: Constants.height
    color: "#ffffff"
    gradient: Gradient {
        GradientStop {
            position: 0.50125
            color: "#ffffff"
        }

        GradientStop {
            position: 1
            color: "#41cd52"
        }
    }

    Text {
        id: pageTitle
        text: qsTr("Qt Account")
        anchors.top: parent.top
        font.pixelSize: 24
        anchors.topMargin: 70
        anchors.horizontalCenter: parent.horizontalCenter
        font.bold: true
        font.family: Constants.font.family
    }

    Image {
        id: logo
        anchors.left: parent.left
        anchors.top: parent.top
        source: "qt_logo_green_64x64px.png"
        anchors.topMargin: 10
        anchors.leftMargin: 10
        fillMode: Image.PreserveAspectFit
    }

    Column {
        id: buttonColumn
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottomMargin: 50
        spacing: 5

        PushButton {
            id: loginButton
            width: 120
            opacity: 1
            text: qsTr("Log In")
        }

        PushButton {
            id: registerButton
            width: 120
            text: qsTr("Create Account")
            font.bold: true

            Connections {
                target: registerButton
                onClicked: rectangle.state = "registerState"
            }
        }
    }

    PushButton {
        id: backButton
        width: 40
        opacity: 1.2
        text: "<"
        anchors.right: parent.right
        anchors.top: parent.top
        font.pixelSize: 24
        anchors.rightMargin: 10
        anchors.topMargin: 10
        font.bold: true
        checked: true

        Connections {
            target: backButton
            onClicked: rectangle.state = "loginState"
        }
    }

    TextField {
        id: verifyPasswordField
        x: 170
        width: 300
        opacity: 1
        anchors.top: passwordField.bottom
        anchors.horizontalCenter: passwordField.horizontalCenter
        anchors.topMargin: 5
        placeholderText: qsTr("Verify password")
    }

    TextField {
        id: passwordField
        x: 170
        width: 300
        anchors.top: usernameField.bottom
        anchors.horizontalCenter: usernameField.horizontalCenter
        anchors.topMargin: 5
        placeholderText: qsTr("Password")
    }

    TextField {
        id: usernameField
        x: 170
        width: 300
        text: ""
        anchors.top: parent.top
        horizontalAlignment: Text.AlignLeft
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.topMargin: 200
        placeholderText: qsTr("Username")
    }

    Timeline {
        id: timeline
        animations: [
            TimelineAnimation {
                id: toLoginState
                loops: 1
                duration: 1000
                running: false
                to: 1000
                from: 0
            },
            TimelineAnimation {
                id: toRegisterState
                loops: 1
                duration: 1000
                running: false
                to: 0
                from: 1000
            }
        ]
        enabled: true
        endFrame: 1000
        startFrame: 0

        KeyframeGroup {
            target: backButton
            property: "opacity"
            Keyframe {
                frame: 0
                value: 0
            }

            Keyframe {
                frame: 1000
                value: 1
            }
        }

        KeyframeGroup {
            target: verifyPasswordField
            property: "opacity"

            Keyframe {
                frame: 0
                value: 0
            }

            Keyframe {
                frame: 1000
                value: 1
            }
        }

        KeyframeGroup {
            target: loginButton
            property: "opacity"
            Keyframe {
                frame: 0
                value: 1
            }

            Keyframe {
                frame: 1000
                value: 0
            }
        }

        KeyframeGroup {
            target: verifyPasswordField
            property: "anchors.topMargin"
            Keyframe {
                frame: 0
                value: -40
            }

            Keyframe {
                easing.bezierCurve: [0.39,0.575,0.565,1,1,1]
                frame: 1000
                value: 5
            }
        }
    }
    states: [
        State {
            name: "loginState"

            PropertyChanges {
                target: timeline
                currentFrame: 0
                enabled: true
            }

            PropertyChanges {
                target: toLoginState
            }

            PropertyChanges {
                target: toRegisterState
                running: true
            }
        },
        State {
            name: "registerState"

            PropertyChanges {
                target: timeline
                enabled: true
            }

            PropertyChanges {
                target: toLoginState
                running: true
            }
        }
    ]
}

/*##^##
Designer {
    D{i:0;formeditorZoom:0.5}D{i:5}D{i:7}D{i:10}D{i:12}D{i:13}D{i:14}D{i:15}
}
##^##*/

