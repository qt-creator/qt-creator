// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import StudioControls 1.0 as StudioControls

Item {
    id: wrapper

    property alias decimals: spinBox.decimals
    property alias hasSlider: spinBox.hasSlider
    property alias value: spinBox.realValue
    property alias minimumValue: spinBox.realFrom
    property alias maximumValue: spinBox.realTo
    property alias stepSize: spinBox.realStepSize
    property alias spinBoxIndicatorVisible: spinBox.spinBoxIndicatorVisible
    property alias sliderIndicatorVisible: spinBox.sliderIndicatorVisible
    property alias hover: spinBox.hover
    property alias inputHAlignment: spinBox.inputHAlignment

    property alias pixelsPerUnit: spinBox.pixelsPerUnit

    signal valueModified
    signal dragStarted
    signal indicatorPressed

    width: 90
    implicitHeight: spinBox.height

    onFocusChanged: restoreCursor()

    StudioControls.RealSpinBox {
        id: spinBox

        __devicePixelRatio: devicePixelRatio()

        onDragStarted: {
            hideCursor()
            wrapper.dragStarted()
        }
        onDragEnded: restoreCursor()
        onDragging: holdCursorInPlace()
        onIndicatorPressed: wrapper.indicatorPressed()

        property bool hasSlider: spinBox.sliderIndicatorVisible

        width: wrapper.width
        actionIndicatorVisible: false

        realFrom: 0.0
        realTo: 1.0
        realStepSize: 0.1
        decimals: 2

        onRealValueModified: wrapper.valueModified()
    }
}
