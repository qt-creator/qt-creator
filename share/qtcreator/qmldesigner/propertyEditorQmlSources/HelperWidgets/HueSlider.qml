/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of the Qt Graphical Effects module.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
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
**   * Neither the name of The Qt Company Ltd and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
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

import QtQuick 2.1

Item {
    id: colorSlider

    property real value: 1
    property real maximum: 1
    property real minimum: 0
    property bool pressed: mouseArea.pressed
    property bool integer: false
    property Component trackDelegate
    property string handleSource: "images/slider_handle.png"

    signal clicked

    width: 20
    height: 100

    function updatePos() {
        if (maximum > minimum) {
            var pos = (track.height - 8) * (value - minimum) / (maximum - minimum)
            return Math.min(Math.max(pos, 0), track.height - 8);
        } else {
            return 0;
        }
    }

    Row {
        height: parent.height
        spacing: 12

        Item {
            id: track
            width: 6
            anchors.top: parent.top
            anchors.bottom: parent.bottom

            Rectangle {
                anchors.fill: track
                anchors.margins: -1
                color: "darkGray"
            }

            Rectangle {
                gradient: Gradient {
                    GradientStop {position: 0.000; color: Qt.rgba(1, 0, 0, 1)}
                    GradientStop {position: 0.167; color: Qt.rgba(1, 1, 0, 1)}
                    GradientStop {position: 0.333; color: Qt.rgba(0, 1, 0, 1)}
                    GradientStop {position: 0.500; color: Qt.rgba(0, 1, 1, 1)}
                    GradientStop {position: 0.667; color: Qt.rgba(0, 0, 1, 1)}
                    GradientStop {position: 0.833; color: Qt.rgba(1, 0, 1, 1)}
                    GradientStop {position: 1.000; color: Qt.rgba(1, 0, 0, 1)}
                }

                width: parent.width
                height: parent.height
            }

            Rectangle {
                id: handle
                width: 14
                height: 10

                opacity: 0.9

                anchors.horizontalCenter: parent.horizontalCenter
                smooth: true

                radius: 2
                border.color: "black"
                border.width: 1

                gradient: Gradient {
                    GradientStop {color: "#2c2c2c" ; position: 0}
                    GradientStop {color: "#343434" ; position: 0.15}
                    GradientStop {color: "#373737" ; position: 1.0}
                }


                y: updatePos()
                z: 1
            }

            MouseArea {
                id: mouseArea
                anchors {top: parent.top; bottom: parent.bottom; horizontalCenter: parent.horizontalCenter}

                width: handle.width
                preventStealing: true

                onPressed: {
                    var handleY = Math.max(0, Math.min(mouseY, mouseArea.height))
                    var realValue = (maximum - minimum) * handleY / mouseArea.height + minimum;
                    value = colorSlider.integer ? Math.round(realValue) : realValue;
                }

                onReleased: colorSlider.clicked()

                onPositionChanged: {
                    if (pressed) {
                        var handleY = Math.max(0, Math.min(mouseY, mouseArea.height))
                        var realValue = (maximum - minimum) * handleY / mouseArea.height + minimum;
                        value = colorSlider.integer ? Math.round(realValue) : realValue;
                    }
                }
            }
        }
    }
}
