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
        stepSize: .01
        decimals: 2
        onValueModified: {
            uniformValue = value
            root.valueChanged()
        }
    }

    StudioControls.Slider {
        id: slider

        width: parent.width - spinBox.width - root.spacing
        visible: !hideSlider && width > 20
        labels: false
        decimals: 2
        actionIndicatorVisible: false
        handleLabelVisible: false
        from: uniformMinValue
        to: uniformMaxValue
        value: uniformValue
        onMoved: {
            let fixedValue = Number.parseFloat(value).toFixed(slider.decimals)
            uniformValue = fixedValue
            spinBox.value = fixedValue // binding isn't working for this property so update it
            root.valueChanged()
        }
    }
}
