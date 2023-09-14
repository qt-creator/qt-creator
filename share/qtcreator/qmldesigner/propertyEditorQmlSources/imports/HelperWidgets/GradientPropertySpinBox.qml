// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import QtQuick.Layouts 1.15
import StudioControls 1.0 as StudioControls
import StudioTheme 1.0 as StudioTheme

SecondColumnLayout {
    id: wrapper

    property string propertyName
    property string gradientTypeName

    property alias decimals: spinBox.decimals
    property alias value: spinBox.realValue
    property alias minimumValue: spinBox.realFrom
    property alias maximumValue: spinBox.realTo
    property alias stepSize: spinBox.realStepSize

    property alias pixelsPerUnit: spinBox.pixelsPerUnit

    width: 90
    implicitHeight: spinBox.height

    onFocusChanged: restoreCursor()

    property bool __isPercentage: false
    property bool __mightHavePercents: gradientLine.model.isPercentageSupportedByProperty(wrapper.propertyName, wrapper.gradientTypeName)

    function readValue() {
        wrapper.__isPercentage = (gradientLine.model.readGradientPropertyUnits(wrapper.propertyName) === GradientModel.Percentage);

        if (wrapper.__isPercentage) {
            unitType.currentIndex = 1;
            spinBox.realValue = gradientLine.model.readGradientPropertyPercentage(wrapper.propertyName)
        }
        else {
            unitType.currentIndex = 0;
            spinBox.realValue = gradientLine.model.readGradientProperty(wrapper.propertyName)
        }
    }

    StudioControls.RealSpinBox {
        id: spinBox

        __devicePixelRatio: devicePixelRatio()

        implicitWidth: StudioTheme.Values.colorEditorPopupSpinBoxWidth * 1.5
        width: implicitWidth
        actionIndicatorVisible: false

        realFrom: -9999
        realTo: 9999
        realStepSize: wrapper.__isPercentage ? 0.1 : 1
        decimals: wrapper.__isPercentage ? 4 : 0

        Component.onCompleted: wrapper.readValue()
        onCompressedRealValueModified: {
            if (wrapper.__isPercentage)
                gradientLine.model.setGradientPropertyPercentage(wrapper.propertyName, spinBox.realValue)
            else
                gradientLine.model.setGradientProperty(wrapper.propertyName, spinBox.realValue)
        }

        onDragStarted: hideCursor()
        onDragEnded: restoreCursor()
        onDragging: holdCursorInPlace()
    }

    Spacer {
        implicitWidth: StudioTheme.Values.twoControlColumnGap
    }

    StudioControls.ComboBox {
        id: unitType
        implicitWidth: StudioTheme.Values.colorEditorPopupSpinBoxWidth
        width: implicitWidth
        model: ["px", "%"] //px = 0, % = 1
        actionIndicatorVisible: false
        visible: wrapper.__mightHavePercents

        onActivated: {
            if (!wrapper.__mightHavePercents)
                return

            if (unitType.currentIndex === 0)
                gradientLine.model.setGradientPropertyUnits(wrapper.propertyName, GradientModel.Pixels)
            else
                gradientLine.model.setGradientPropertyUnits(wrapper.propertyName, GradientModel.Percentage)

            wrapper.readValue()
        }
    }

    ExpandingSpacer {}
}
