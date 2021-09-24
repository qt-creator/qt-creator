/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator
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

import QtQuick 2.15
import QtQuick.Window 2.15

Rectangle {
    id: page
    state: "State1"

    Image {
        id: icon
        x: 20
        y: 20
        source: "images/qt-logo.png"
        fillMode: Image.PreserveAspectFit
    }

    Rectangle {
        id: topLeftRect
        width: 55
        height: 41
        color: "#00ffffff"
        border.color: "#808080"
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.leftMargin: 20
        anchors.topMargin: 20

        MouseArea {
            id: mouseArea
            anchors.fill: parent

            Connections {
                target: mouseArea
                function onClicked()
                {
                    page.state = "State1"
                }
            }
        }
    }


    Rectangle {
        id: middleRightRect
        width: 55
        height: 41
        color: "#00ffffff"
        border.color: "#808080"
        anchors.verticalCenter: parent.verticalCenter
        anchors.right: parent.right
        anchors.rightMargin: 20
        MouseArea {
            id: mouseArea1
            anchors.fill: parent

            Connections {
                target: mouseArea1
                function onClicked()
                {
                    page.state = "State2"
                }
            }
        }
    }


    Rectangle {
        id: bottomLeftRect
        width: 55
        height: 41
        color: "#00ffffff"
        border.color: "#808080"
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 20
        anchors.leftMargin: 20
        MouseArea {
            id: mouseArea2
            anchors.fill: parent

            Connections {
                target: mouseArea2
                function onClicked()
                {
                    page.state = "State3"
                }
            }
        }
    }

    states: [
        State {
            name: "State1"
        },
        State {
            name: "State2"

            PropertyChanges {
                target: icon
                x: middleRightRect.x
                y: middleRightRect.y
            }
        },
        State {
            name: "State3"

            PropertyChanges {
                target: icon
                x: bottomLeftRect.x
                y: bottomLeftRect.y
            }
        }
    ]
    transitions: [
        Transition {
            id: toState1
            ParallelAnimation {
                SequentialAnimation {
                    PauseAnimation {
                        duration: 0
                    }

                    PropertyAnimation {
                        target: icon
                        property: "y"
                        duration: 200
                    }
                }

                SequentialAnimation {
                    PauseAnimation {
                        duration: 0
                    }

                    PropertyAnimation {
                        target: icon
                        property: "x"
                        duration: 200
                    }
                }
            }
            to: "State1"
            from: "State2,State3"
        },
        Transition {
            id: toState2
            ParallelAnimation {
                SequentialAnimation {
                    PauseAnimation {
                        duration: 0
                    }

                    PropertyAnimation {
                        target: icon
                        property: "y"
                        easing.bezierCurve: [0.233,0.161,0.264,0.997,0.393,0.997,0.522,0.997,0.555,0.752,0.61,0.75,0.664,0.748,0.736,1,0.775,1,0.814,0.999,0.861,0.901,0.888,0.901,0.916,0.901,0.923,0.995,1,1]
                        duration: 1006
                    }
                }

                SequentialAnimation {
                    PauseAnimation {
                        duration: 0
                    }

                    PropertyAnimation {
                        target: icon
                        property: "x"
                        easing.bezierCurve: [0.233,0.161,0.264,0.997,0.393,0.997,0.522,0.997,0.555,0.752,0.61,0.75,0.664,0.748,0.736,1,0.775,1,0.814,0.999,0.861,0.901,0.888,0.901,0.916,0.901,0.923,0.995,1,1]
                        duration: 1006
                    }
                }
            }
            to: "State2"
            from: "State1,State3"
        },
        Transition {
            id: toState3
            ParallelAnimation {
                SequentialAnimation {
                    PauseAnimation {
                        duration: 0
                    }

                    PropertyAnimation {
                        target: icon
                        property: "y"
                        easing.bezierCurve: [0.455,0.03,0.515,0.955,1,1]
                        duration: 2000
                    }
                }

                SequentialAnimation {
                    PauseAnimation {
                        duration: 0
                    }

                    PropertyAnimation {
                        target: icon
                        property: "x"
                        easing.bezierCurve: [0.455,0.03,0.515,0.955,1,1]
                        duration: 2000
                    }
                }
            }
            to: "State3"
            from: "State1,State2"
        }
    ]
}

/*##^##
Designer {
    D{i:0;height:480;width:640}D{i:1}D{i:16;transitionDuration:2000}D{i:24;transitionDuration:2000}
D{i:32;transitionDuration:2000}
}
##^##*/
