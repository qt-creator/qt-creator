// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Layouts
import StudioControls as StudioControls
import StudioTheme as StudioTheme
import EffectComposerBackend

RowLayout {
    width: parent.width
    spacing: 0

    StudioControls.RealSpinBox {
        id: vX

        Layout.fillWidth: true
        Layout.minimumWidth: 30
        Layout.maximumWidth: 60

        actionIndicatorVisible: false
        spinBoxIndicatorVisible: false
        inputHAlignment: Qt.AlignHCenter
        realFrom: uniformMinValue.x
        realTo: uniformMaxValue.x
        realValue: uniformValue.x
        realStepSize: .01
        decimals: 2
        onRealValueModified: uniformValue.x = realValue
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

    StudioControls.RealSpinBox {
        id: vY

        Layout.fillWidth: true
        Layout.minimumWidth: 30
        Layout.maximumWidth: 60

        actionIndicatorVisible: false
        spinBoxIndicatorVisible: false
        inputHAlignment: Qt.AlignHCenter
        realFrom: uniformMinValue.y
        realTo: uniformMaxValue.y
        realValue: uniformValue.y
        realStepSize: .01
        decimals: 2
        onRealValueModified: uniformValue.y = realValue
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

    StudioControls.RealSpinBox {
        id: vZ

        Layout.fillWidth: true
        Layout.minimumWidth: 30
        Layout.maximumWidth: 60

        actionIndicatorVisible: false
        spinBoxIndicatorVisible: false
        inputHAlignment: Qt.AlignHCenter
        realFrom: uniformMinValue.z
        realTo: uniformMaxValue.z
        realValue: uniformValue.z
        realStepSize: .01
        decimals: 2
        onRealValueModified: uniformValue.z = realValue
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
    }

}
