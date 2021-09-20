

/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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
import QtQuick
import QtQuick.Controls
import loginui3 1.0

Rectangle {
    id: rectangle
    width: Constants.width
    height: Constants.height

    color: Constants.backgroundColor
    state: "login"

    Image {
        id: adventurePage
        anchors.fill: parent
        source: "images/adventurePage.jpg"
        fillMode: Image.PreserveAspectFit
    }

    Image {
        id: qt_logo_green_128x128px
        x: 296
        anchors.top: parent.top
        source: "images/qt_logo_green_128x128px.png"
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.topMargin: 40
        fillMode: Image.PreserveAspectFit
    }
    Text {
        id: tagLine
        width: 541
        height: 78
        color: "#ffffff"
        text: qsTr("Are you ready to explore?")
        anchors.top: qt_logo_green_128x128px.bottom
        font.pixelSize: 50
        anchors.topMargin: 40
        anchors.horizontalCenter: parent.horizontalCenter
        font.family: "Titillium Web ExtraLight"
    }

    Column {
        id: fields
        x: 128
        anchors.top: tagLine.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.topMargin: 170
        spacing: 20

        EntryField {
            id: username
            text: "Username or Email"
        }

        EntryField {
            id: password
            text: qsTr("Password")
        }

        EntryField {
            id: repeatPassword
            text: "Repeat Password"
        }
    }

    Column {
        id: buttons
        x: 102
        y: 966
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottomMargin: 100
        spacing: 20

        PushButton {
            id: login
            text: qsTr("Continue")
        }

        PushButton {
            id: createAccount
            text: qsTr("Create Account")

            Connections {
                target: createAccount
                onClicked: rectangle.state = "createAccount"
            }
        }
    }
    states: [
        State {
            name: "login"

            PropertyChanges {
                target: repeatPassword
                visible: false
            }
        },
        State {
            name: "createAccount"

            PropertyChanges {
                target: createAccount
                visible: false
            }
        }
    ]
}

/*##^##
Designer {
    D{i:0;formeditorZoom:0.5}
}
##^##*/

