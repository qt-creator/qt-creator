// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import StudioControls as StudioControls
import StudioTheme as StudioTheme
import EffectComposerBackend

Row {
    width: parent.width
    spacing: 5

    StudioControls.SpinBox {
        id: spinBox

        width: 60
        actionIndicatorVisible: false
        spinBoxIndicatorVisible: false
        inputHAlignment: Qt.AlignHCenter
        from: uniformMinValue
        to: uniformMaxValue
        value: uniformValue
        onValueModified: uniformValue = value
    }

    StudioControls.Slider {
        id: slider

        width: parent.width - 100
        visible: width > 20
        labels: false
        actionIndicatorVisible: false
        handleLabelVisible: false
        from: uniformMinValue
        to: uniformMaxValue
        value: uniformValue
        onMoved: {
            uniformValue = Math.round(value)
            spinBox.value = Math.round(value)
        }
    }
}
