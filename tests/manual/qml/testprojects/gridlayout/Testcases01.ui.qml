// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.0
import QtQuick.Layouts 1.0
import QtQuick.Controls 1.2

Rectangle {
    width: 640
    height: 480

    Label {
        id: label1
        x: 8
        y: 11
        text: qsTr("Name")
    }

    Label {
        id: label2
        x: 8
        y: 43
        text: qsTr("Status")
    }

    Label {
        id: label3
        x: 8
        y: 62
        text: qsTr("Type")
    }

    Label {
        id: label4
        x: 8
        y: 81
        text: qsTr("Where")
    }

    Label {
        id: label5
        x: 8
        y: 100
        text: qsTr("Comment")
    }

    ComboBox {
        id: comboBox1
        x: 63
        y: 8
        width: 171
        height: 20
    }

    Button {
        id: button1
        x: 247
        y: 6
        text: qsTr("Button")
    }

    Label {
        id: label6
        x: 63
        y: 43
        text: qsTr("Idle")
    }

    Label {
        id: label7
        x: 63
        y: 62
        text: qsTr("HP Deskjet")
    }

    Label {
        id: label8
        x: 63
        y: 81
        text: qsTr("LPT 1")
    }

    CheckBox {
        id: checkBox1
        x: 247
        y: 69
        text: qsTr("Print to file")
    }

    RadioButton {
        id: radioButton1
        x: 8
        y: 198
        text: qsTr("All")
    }

    RadioButton {
        id: radioButton2
        x: 8
        y: 221
        text: qsTr("Current page")
    }

    RadioButton {
        id: radioButton3
        x: 114
        y: 221
        text: qsTr("Selection")
    }

    RadioButton {
        id: radioButton4
        x: 8
        y: 244
        text: qsTr("Pages")
    }

    TextField {
        id: textField1
        x: 114
        y: 244
        placeholderText: qsTr("Text Field")
    }

    Label {
        id: label9
        x: 10
        y: 280
        width: 231
        height: 13
        text: qsTr("Some long description covering several lines with long explanations")
        wrapMode: Text.WordWrap
    }
}

