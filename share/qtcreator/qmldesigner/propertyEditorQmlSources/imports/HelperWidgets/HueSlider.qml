/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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
