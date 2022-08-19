// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick 2.8

Item {
    id: timedate
    width: 47
    height: 29

    readonly property alias currentHourInt: timeContainer.hrsInt
    readonly property alias currentMinuteInt: timeContainer.minInt

    Text {
        id: timelabel
        x: 2
        y: -1
        width: 43
        height: 16
        color: "#B8B8B8"
        font.pixelSize: 16
        horizontalAlignment: Text.AlignHCenter
        font.family: "Maven Pro"

        text: timeContainer.hrsStr + ":" + timeContainer.minStr
    }


    Item {
        id: timeContainer
        property string hrsStr: "00"
        property string minStr: "00"

        property int hrsInt: 0
        property int minInt: 0

        Timer {
            id: timer
            interval: 1000
            running: true
            repeat: true

            onTriggered: {
                updateTime()
            }

            function updateTime() {
                var currentDate = new Date()
                timeContainer.hrsInt = currentDate.getHours()
                if (timeContainer.hrsInt < 10) timeContainer.hrsStr = "0" + timeContainer.hrsInt
                else timeContainer.hrsStr = timeContainer.hrsInt

                timeContainer.minInt = currentDate.getMinutes()
                if (timeContainer.minInt < 10) timeContainer.minStr = "0" + timeContainer.minInt
                else timeContainer.minStr = timeContainer.minInt
            }
        }
    }

    Component.onCompleted: {
        timer.updateTime()
    }
}
