// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import QtQuick.Layouts 1.15
import StudioControls 1.0 as StudioControls

Item {
    id: wrapper

    property string propertyName

    property alias decimals: spinBox.decimals
    property alias value: spinBox.realValue
    property alias minimumValue: spinBox.realFrom
    property alias maximumValue: spinBox.realTo
    property alias stepSize: spinBox.realStepSize

    property alias pixelsPerUnit: spinBox.pixelsPerUnit

    width: 90
    implicitHeight: spinBox.height

    onFocusChanged: restoreCursor()

    function readValue() {
        spinBox.realValue = gradientLine.model.readGradientProperty(wrapper.propertyName)
    }

    StudioControls.RealSpinBox {
        id: spinBox

        __devicePixelRatio: devicePixelRatio()

        width: wrapper.width
        actionIndicatorVisible: false

        realFrom: -9999
        realTo: 9999
        realStepSize: 1
        decimals: 0

        Component.onCompleted: wrapper.readValue()
        onCompressedRealValueModified: {
            gradientLine.model.setGradientProperty(wrapper.propertyName, spinBox.realValue)
        }

        onDragStarted: hideCursor()
        onDragEnded: restoreCursor()
        onDragging: holdCursorInPlace()
    }
}
