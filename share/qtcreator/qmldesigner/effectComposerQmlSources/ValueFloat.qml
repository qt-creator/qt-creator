// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import StudioControls as StudioControls
import StudioTheme as StudioTheme
import EffectComposerBackend

Row {
    width: parent.width
    spacing: 5

    StudioControls.RealSpinBox {
        id: spinBox

        width: 60
        actionIndicatorVisible: false
        spinBoxIndicatorVisible: false
        inputHAlignment: Qt.AlignHCenter
        realFrom: uniformMinValue
        realTo: uniformMaxValue
        realValue: uniformValue
        realStepSize: .01
        decimals: 2
        onRealValueModified: uniformValue = realValue
    }

    StudioControls.Slider {
        id: slider

        width: parent.width - 100
        visible: width > 20
        labels: false
        decimals: 2
        actionIndicatorVisible: false
        handleLabelVisible: false
        from: uniformMinValue
        to: uniformMaxValue
        value: uniformValue
        onMoved: {
            uniformValue = value
            spinBox.realValue = value // binding isn't working for this property so update it
        }
    }
}
