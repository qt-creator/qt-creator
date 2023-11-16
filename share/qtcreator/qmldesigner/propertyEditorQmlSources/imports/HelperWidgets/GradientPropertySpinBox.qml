// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import StudioControls as StudioControls
import StudioTheme as StudioTheme

Row {
    id: root

    property string propertyName
    property string gradientTypeName

    property alias decimals: spinBox.decimals
    property alias value: spinBox.realValue
    property alias minimumValue: spinBox.realFrom
    property alias maximumValue: spinBox.realTo
    property alias stepSize: spinBox.realStepSize

    property alias pixelsPerUnit: spinBox.pixelsPerUnit

    property real spinBoxWidth: 100
    property real unitWidth: 50

    spacing: StudioTheme.Values.controlGap

    onFocusChanged: restoreCursor()

    property bool __isPercentage: false
    property bool __mightHavePercents: gradientLine.model.isPercentageSupportedByProperty(root.propertyName, root.gradientTypeName)

    function readValue() {
        root.__isPercentage = (gradientLine.model.readGradientPropertyUnits(root.propertyName) === GradientModel.Percentage);

        if (root.__isPercentage) {
            unitType.currentIndex = 1;
            spinBox.realValue = gradientLine.model.readGradientPropertyPercentage(root.propertyName)
        }
        else {
            unitType.currentIndex = 0;
            spinBox.realValue = gradientLine.model.readGradientProperty(root.propertyName)
        }
    }

    StudioControls.RealSpinBox {
        id: spinBox

        __devicePixelRatio: devicePixelRatio()

        implicitWidth: root.spinBoxWidth
        width: spinBox.implicitWidth
        actionIndicatorVisible: false

        realFrom: -9999
        realTo: 9999
        realStepSize: root.__isPercentage ? 0.1 : 1
        decimals: root.__isPercentage ? 4 : 0

        Component.onCompleted: root.readValue()
        onCompressedRealValueModified: {
            if (root.__isPercentage)
                gradientLine.model.setGradientPropertyPercentage(root.propertyName, spinBox.realValue)
            else
                gradientLine.model.setGradientProperty(root.propertyName, spinBox.realValue)
        }

        onDragStarted: hideCursor()
        onDragEnded: restoreCursor()
        onDragging: holdCursorInPlace()
    }

    StudioControls.ComboBox {
        id: unitType
        implicitWidth: root.unitWidth
        width: unitType.implicitWidth
        model: ["px", "%"] //px = 0, % = 1
        actionIndicatorVisible: false
        visible: root.__mightHavePercents

        onActivated: {
            if (!root.__mightHavePercents)
                return

            if (unitType.currentIndex === 0)
                gradientLine.model.setGradientPropertyUnits(root.propertyName, GradientModel.Pixels)
            else
                gradientLine.model.setGradientPropertyUnits(root.propertyName, GradientModel.Percentage)

            root.readValue()
        }
    }
}
