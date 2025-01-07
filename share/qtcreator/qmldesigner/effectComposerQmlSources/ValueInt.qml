// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import HelperWidgets as HelperWidgets
import StudioControls as StudioControls

Row {
    id: root

    property bool hideSlider: false

    width: parent.width
    spacing: 5

    signal valueChanged()

    HelperWidgets.DoubleSpinBox {
        id: spinBox

        // value: uniformValue binding can get overwritten by normal operation of the control
        property double resetValue: uniformValue
        onResetValueChanged: value = resetValue

        width: 60
        spinBoxIndicatorVisible: false
        inputHAlignment: Qt.AlignHCenter
        minimumValue: uniformMinValue
        maximumValue: uniformMaxValue
        value: uniformValue
        stepSize: 1
        decimals: 0
        onValueModified: {
            uniformValue = Math.round(value)
            root.valueChanged()
        }
    }

    StudioControls.Slider {
        id: slider

        width: parent.width - spinBox.width - root.spacing
        visible: !hideSlider && width > 20
        labels: false
        actionIndicatorVisible: false
        handleLabelVisible: false
        from: uniformMinValue
        to: uniformMaxValue
        value: uniformValue
        onMoved: {
            uniformValue = Math.round(value)
            spinBox.value = Math.round(value)
            root.valueChanged()
        }
    }
}
