
// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
import QtQuick
import QtQuick.Controls
import StudioControls

Rectangle {
    width: 400
    height: 800
    color: "#1b1b1b"

    Text {
        id: text1
        x: 10
        y: 25
        color: "#ffffff"
        text: qsTr("Target:")
        font.pixelSize: 15
    }

    TopLevelComboBox {
        id: target
        x: 95
        width: 210
        anchors.verticalCenter: parent.verticalCenter
        anchors.verticalCenterOffset: -367
        model: ["mySpinbox", "foo", "backendObject"]
    }

    Text {
        id: text2
        x: 10
        y: 131
        color: "#ffffff"
        text: qsTr("Signal")
        font.pixelSize: 15
    }

    TopLevelComboBox {
        id: signal
        x: 10
        y: 7
        width: 156
        anchors.verticalCenter: parent.verticalCenter
        anchors.verticalCenterOffset: -207
        model: ["onClicked", "onPressed", "onReleased"]
    }

    TopLevelComboBox {
        id: action
        x: 207
        y: 7
        width: 156
        anchors.verticalCenter: parent.verticalCenter
        anchors.verticalCenterOffset: -207
        model: ["Call Function", "Assign", "ChnageState"]
    }

    Text {
        id: text3
        x: 207
        y: 131
        color: "#ffffff"
        text: qsTr("Action")
        font.pixelSize: 15
    }

    Item {
        id: functionGroup
        x: 0
        y: 276
        width: 400
        height: 176

        Text {
            id: text4
            x: 17
            y: -11
            color: "#ffffff"
            text: qsTr("Target")
            font.pixelSize: 15
        }

        TopLevelComboBox {
            id: functionTarget
            x: 10
            y: 7
            width: 156
            anchors.verticalCenter: parent.verticalCenter
            anchors.verticalCenterOffset: -48
            model: ["mySpinBox", "backendObject", "someButton"]
        }

        TopLevelComboBox {
            id: functionName
            x: 203
            y: 7
            width: 156
            anchors.verticalCenter: parent.verticalCenter
            anchors.verticalCenterOffset: -48
            model: ["start", "trigger", "stop"]
        }

        Text {
            id: text5
            x: 203
            y: -11
            color: "#ffffff"
            text: qsTr("Function")
            font.pixelSize: 15
        }
    }

    Item {
        id: statesGroup
        x: 0
        y: 383
        width: 400
        height: 106
        Text {
            id: text6
            x: 17
            y: -11
            color: "#ffffff"
            text: qsTr("State Group")
            font.pixelSize: 15
        }

        TopLevelComboBox {
            id: stateGroup
            x: 10
            y: 7
            width: 156
            anchors.verticalCenter: parent.verticalCenter
            anchors.verticalCenterOffset: -12
            model: ["default", "State Group 01", "State Group 02"]
        }

        TopLevelComboBox {
            id: stateName
            x: 209
            y: 7
            width: 156
            anchors.verticalCenter: parent.verticalCenter
            anchors.verticalCenterOffset: -12
            model: ["State 01", "State 02", "State 03"]
        }

        Text {
            id: text7
            x: 209
            y: -11
            color: "#ffffff"
            text: qsTr("State")
            font.pixelSize: 15
        }
    }

    Item {
        id: assignment
        x: 10
        y: 505
        width: 400
        height: 106
        Text {
            id: text8
            x: 17
            y: -11
            color: "#ffffff"
            text: qsTr("target")
            font.pixelSize: 15
        }

        TopLevelComboBox {
            id: valueTarget
            x: 10
            y: 7
            width: 156
            anchors.verticalCenter: parent.verticalCenter
            anchors.verticalCenterOffset: -12
            model: ["mySpinBox", "myButton", "backendObject"]
        }

        TopLevelComboBox {
            id: valueSource
            x: 209
            y: 7
            width: 156
            anchors.verticalCenter: parent.verticalCenter
            anchors.verticalCenterOffset: -12
            model: ["mySpinBox", "myButton", "backendObject"]
        }

        Text {
            id: text9
            x: 209
            y: -11
            color: "#ffffff"
            text: qsTr("source")
            font.pixelSize: 15
        }

        Text {
            id: text10
            x: 17
            y: 76
            color: "#ffffff"
            text: qsTr("value")
            font.pixelSize: 15
        }

        TopLevelComboBox {
            id: valueOut
            x: 10
            y: -2
            width: 156
            anchors.verticalCenter: parent.verticalCenter
            anchors.verticalCenterOffset: 84
            model: ["width", "height", "opacity"]
        }

        TopLevelComboBox {
            id: valueIn
            x: 209
            y: -2
            width: 156
            anchors.verticalCenter: parent.verticalCenter
            anchors.verticalCenterOffset: 84
            model: ["width", "height", "x", "y"]
        }

        Text {
            id: text11
            x: 209
            y: 76
            color: "#ffffff"
            text: qsTr("value")
            font.pixelSize: 15
        }
    }
}
