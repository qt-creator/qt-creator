/****************************************************************************
**
** Copyright (c) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator
**
**
** GNU Free Documentation License
**
** Alternatively, this file may be used under the terms of the GNU Free
** Documentation License version 1.3 as published by the Free Software
** Foundation and appearing in the file included in the packaging of this
** file.
**
**
****************************************************************************/

// **********************************************************************
// NOTE: the sections are not ordered by their logical order to avoid
// reshuffling the file each time the index order changes (i.e., often).
// Run the fixnavi.pl script to adjust the links to the index order.
// **********************************************************************

import QtQuick 2.1
import QtQuick.Controls 1.0

import QtSensors 5.0

ApplicationWindow {
    title: qsTr("Accelerate Bubble")
    id: mainWindow
    width: 640
    height: 480
    visible: true

    Accelerometer {
        id: accel
        dataRate: 100
        active:true

        onReadingChanged: {
            var newX = (bubble.x + calcRoll(accel.reading.x, accel.reading.y, accel.reading.z) * 0.1)
            var newY = (bubble.y - calcPitch(accel.reading.x, accel.reading.y, accel.reading.z) * 0.1)

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

    Image {
        id: bubble
        source: "Bluebubble.svg"
        smooth: true
        property real centerX: mainWindow.width / 2
        property real centerY: mainWindow.height / 2
        property real bubbleCenter: bubble.width / 2
        x: centerX - bubbleCenter
        y: centerY - bubbleCenter

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

    MenuBar {
        Menu {
            title: qsTr("File")
            MenuItem {
                text: qsTr("Exit")
                onTriggered: Qt.quit();
                }
            }
        }
    }
}
