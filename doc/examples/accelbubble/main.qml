/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

import QtQuick 2.7
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.0
import QtSensors 5.7

ApplicationWindow {
    visible: true
    width: 640
    height: 480
    title: qsTr("Accelerate Bubble")

    Page1 {
        anchors.fill: parent
        bubble {
            id: bubble
            centerX: mainWindow.width / 2
            centerY: mainWindow.height / 2
            bubbleCenter: bubble.width / 2
            x: bubble.centerX - bubble.bubbleCenter
            y: bubble.centerY - bubble.bubbleCenter

            Behavior on y {
                SmoothedAnimation {
                    easing.type: Easing.Linear
                    duration: 100
                }
            }
            Behavior on x {
                SmoothedAnimation {
                    easing.type: Easing.Linear
                    duration: 100
                }
            }
        }
    }

    Accelerometer {
           id: accel
           dataRate: 100
           active: true

           onReadingChanged: {
               var newX = (bubble.x + calcRoll(accel.reading.x, accel.reading.y, accel.reading.z) * 0.1)
               var newY = (bubble.y - calcPitch(accel.reading.x, accel.reading.y, accel.reading.z) * 0.1)

               if (isNaN(newX) || isNaN(newY))
                   return;

               if (newX < 0)
                   newX = 0

               if (newX > mainWindow.width - bubble.width)
                   newX = mainWindow.width - bubble.width

               if (newY < 18)
                   newY = 18

               if (newY > mainWindow.height - bubble.height)
                   newY = mainWindow.height - bubble.height

                   bubble.x = newX
                   bubble.y = newY
           }
       }

       function calcPitch(x, y, z) {
           return -(Math.atan(y / Math.sqrt(x * x + z * z)) * 57.2957795);
       }
       function calcRoll(x, y, z) {
           return -(Math.atan(x / Math.sqrt(y * y + z * z)) * 57.2957795);
       }
}
