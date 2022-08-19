// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick 2.8

Item {
    id: start
    width: 480
    height: 272

    signal startClicked
    signal settingsClicked

    Flatbackground {
        id: backgroundfull
        anchors.fill: parent
    }

    Timedate {
        id: start_timedateinstance
        x: 425
        y: 8
        width: 47
        height: 30
    }

    Image {
        id: startButton
        anchors.verticalCenter: parent.verticalCenter
        source: "assets/drumcopy_temp.png"
        anchors.horizontalCenter: parent.horizontalCenter

        Text {
            id: startlabel
            width: 80
            height: 37
            color: "#B8B8B8"
            text: "Start"
            anchors.verticalCenter: parent.verticalCenter
            font.pixelSize: 36
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            anchors.horizontalCenter: parent.horizontalCenter
            font.family: "Maven Pro"
        }

        MouseArea {
            id: startMA

            anchors.fill: parent

            Connections {
                target: startMA
                onClicked: startClicked()
            }
        }
    }
}
