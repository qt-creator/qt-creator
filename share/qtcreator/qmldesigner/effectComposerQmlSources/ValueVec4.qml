// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Layouts
import HelperWidgets as HelperWidgets
import StudioControls as StudioControls
import StudioTheme as StudioTheme
import EffectComposerBackend

RowLayout {
    width: parent.width
    spacing: 0

    HelperWidgets.DoubleSpinBox {
        id: vX

        Layout.fillWidth: true
        Layout.minimumWidth: 30
        Layout.maximumWidth: 60

        spinBoxIndicatorVisible: false
        inputHAlignment: Qt.AlignHCenter
        minimumValue: uniformMinValue.x
        maximumValue: uniformMaxValue.x
        value: uniformValue.x
        stepSize: .01
        decimals: 2
        onValueChanged: uniformValue.x = value
    }

    Item { // spacer
        Layout.fillWidth: true
        Layout.minimumWidth: 2
        Layout.maximumWidth: 10
    }

    Text {
        text: qsTr("X")
        color: StudioTheme.Values.themeTextColor
        font.pixelSize: StudioTheme.Values.baseFontSize
        Layout.alignment: Qt.AlignVCenter
    }

    Item { // spacer
        Layout.fillWidth: true
        Layout.minimumWidth: 10
        Layout.maximumWidth: 20
    }

    HelperWidgets.DoubleSpinBox {
        id: vY

        Layout.fillWidth: true
        Layout.minimumWidth: 30
        Layout.maximumWidth: 60

        spinBoxIndicatorVisible: false
        inputHAlignment: Qt.AlignHCenter
        minimumValue: uniformMinValue.y
        maximumValue: uniformMaxValue.y
        value: uniformValue.y
        stepSize: .01
        decimals: 2
        onValueChanged: uniformValue.y = value
    }

    Item { // spacer
        Layout.fillWidth: true
        Layout.minimumWidth: 2
        Layout.maximumWidth: 10
    }

    Text {
        text: qsTr("Y")
        color: StudioTheme.Values.themeTextColor
        font.pixelSize: StudioTheme.Values.baseFontSize
        Layout.alignment: Qt.AlignVCenter
    }

    Item { // spacer
        Layout.fillWidth: true
        Layout.minimumWidth: 10
        Layout.maximumWidth: 20
    }

    HelperWidgets.DoubleSpinBox {
        id: vZ

        Layout.fillWidth: true
        Layout.minimumWidth: 30
        Layout.maximumWidth: 60

        spinBoxIndicatorVisible: false
        inputHAlignment: Qt.AlignHCenter
        minimumValue: uniformMinValue.z
        maximumValue: uniformMaxValue.z
        value: uniformValue.z
        stepSize: .01
        decimals: 2
        onValueChanged: uniformValue.z = value
    }

    Item { // spacer
        Layout.fillWidth: true
        Layout.minimumWidth: 2
        Layout.maximumWidth: 10
    }

    Text {
        text: qsTr("Z")
        color: StudioTheme.Values.themeTextColor
        font.pixelSize: StudioTheme.Values.baseFontSize
        Layout.alignment: Qt.AlignVCenter
    }

    Item { // spacer
        Layout.fillWidth: true
        Layout.minimumWidth: 10
        Layout.maximumWidth: 20
    }

    HelperWidgets.DoubleSpinBox {
        id: vW

        Layout.fillWidth: true
        Layout.minimumWidth: 30
        Layout.maximumWidth: 60

        spinBoxIndicatorVisible: false
        inputHAlignment: Qt.AlignHCenter
        minimumValue: uniformMinValue.w
        maximumValue: uniformMaxValue.w
        value: uniformValue.w
        stepSize: .01
        decimals: 2
        onValueChanged: uniformValue.w = value
    }

    Item { // spacer
        Layout.fillWidth: true
        Layout.minimumWidth: 2
        Layout.maximumWidth: 10
    }

    Text {
        text: qsTr("W")
        color: StudioTheme.Values.themeTextColor
        font.pixelSize: StudioTheme.Values.baseFontSize
        Layout.alignment: Qt.AlignVCenter
    }

    Item { // spacer
        Layout.fillWidth: true
        Layout.minimumWidth: 10
    }

}
