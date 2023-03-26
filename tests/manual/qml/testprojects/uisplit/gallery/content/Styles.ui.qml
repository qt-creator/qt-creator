// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.2
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.1
import QtQuick.Particles 2.0
import QtQuick.Layouts 1.0
import "styles"

Item {
    id: root

    width: 600
    height: 300

    property int columnWidth: 120
    GridLayout {
        rowSpacing: 12
        columnSpacing: 30
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.margins: 30

        Button {
            text: "Push me"
            style: ButtonStyle { }
            implicitWidth: columnWidth
        }
        Button {
            text: "Push me"
            style: MyButtonStyle1 {
            }
            implicitWidth: columnWidth
        }
        Button {
            text: "Push me"
            style: MyButtonStyle2 {

            }

            implicitWidth: columnWidth
        }

        TextField {
            Layout.row: 1
            style: TextFieldStyle { }
            implicitWidth: columnWidth
        }
        TextField {
            style: MyTextFieldStyle1 {
            }
            implicitWidth: columnWidth
        }
        TextField {
            style: MyTextFieldStyle2 {
            }
            implicitWidth: columnWidth
        }

        Slider {
            id: slider1
            Layout.row: 2
            value: 0.5
            implicitWidth: columnWidth
            style: SliderStyle { }
        }
        Slider {
            id: slider2
            value: 0.5
            implicitWidth: columnWidth
            style: MySliderStyle1 {
            }
        }
        Slider {
            id: slider3
            value: 0.5
            implicitWidth: columnWidth
            style: MySliderStyle2 {

            }
        }

        ProgressBar {
            Layout.row: 3
            value: slider1.value
            implicitWidth: columnWidth
            style: ProgressBarStyle { }
        }
        ProgressBar {
            value: slider2.value
            implicitWidth: columnWidth
            style: MyProgressBarStyle1 { }
        }
        ProgressBar {
            value: slider3.value
            implicitWidth: columnWidth
            style: MyProgressBarStyle2 { }
        }

        CheckBox {
            text: "CheckBox"
            style: CheckBoxStyle{}
            Layout.row: 4
            implicitWidth: columnWidth
        }
        RadioButton {
            style: RadioButtonStyle{}
            text: "RadioButton"
            implicitWidth: columnWidth
        }

        ComboBox {
            model: ["Paris", "Oslo", "New York"]
            style: ComboBoxStyle{}
            implicitWidth: columnWidth
        }

        TabView {
            Layout.row: 5
            Layout.columnSpan: 3
            Layout.fillWidth: true
            implicitHeight: 30
            Tab { title: "One" ; Item {}}
            Tab { title: "Two" ; Item {}}
            Tab { title: "Three" ; Item {}}
            Tab { title: "Four" ; Item {}}
            style: TabViewStyle {}
        }

        TabView {
            Layout.row: 6
            Layout.columnSpan: 3
            Layout.fillWidth: true
            implicitHeight: 30
            Tab { title: "One" ; Item {}}
            Tab { title: "Two" ; Item {}}
            Tab { title: "Three" ; Item {}}
            Tab { title: "Four" ; Item {}}
            style: MyTabViewStyle {

            }
        }
    }
}

