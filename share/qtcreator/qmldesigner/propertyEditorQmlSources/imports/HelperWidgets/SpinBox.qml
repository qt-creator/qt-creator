// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import StudioControls 1.0 as StudioControls
import StudioTheme 1.0 as StudioTheme

Item {
    id: wrapper

    property alias decimals: spinBox.decimals
    property alias hasSlider: spinBox.hasSlider

    property alias minimumValue: spinBox.realFrom
    property alias maximumValue: spinBox.realTo
    property alias value: spinBox.realValue
    property alias stepSize: spinBox.realStepSize

    property alias backendValue: spinBox.backendValue
    property alias sliderIndicatorVisible: spinBox.sliderIndicatorVisible

    property alias realDragRange: spinBox.realDragRange
    property alias pixelsPerUnit: spinBox.pixelsPerUnit

    width: 96
    implicitHeight: spinBox.height

    onFocusChanged: {
        restoreCursor()
        transaction.end()
    }

    Component.onCompleted: {
        spinBox.enabled = backendValue ? !isBlocked(backendValue.name) : false
    }

    Connections {
        target: modelNodeBackend
        function onSelectionChanged() {
            spinBox.enabled = backendValue ? !isBlocked(backendValue.name) : false
        }
    }

    onBackendValueChanged: {
        spinBox.enabled = backendValue ? !isBlocked(backendValue.name) : false
    }

    StudioControls.RealSpinBox {
        id: spinBox

        __devicePixelRatio: devicePixelRatio()

        onDragStarted: {
            hideCursor()
            transaction.start()
        }

        onDragEnded: {
            restoreCursor()
            transaction.end()
        }

        // Needs to be held in place due to screen size limits potentially being hit while dragging
        onDragging: holdCursorInPlace()

        onRealValueModified: {
            if (transaction.active())
                spinBox.commitValue()
        }

        function commitValue() {
            if (spinBox.backendValue.value !== spinBox.realValue)
                spinBox.backendValue.value = spinBox.realValue
        }

        property variant backendValue
        property bool hasSlider: wrapper.sliderIndicatorVisible

        width: wrapper.width

        ExtendedFunctionLogic {
            id: extFuncLogic
            backendValue: spinBox.backendValue
        }

        actionIndicator.icon.color: extFuncLogic.color
        actionIndicator.icon.text: extFuncLogic.glyph
        actionIndicator.onClicked: extFuncLogic.show()
        actionIndicator.forceVisible: extFuncLogic.menuVisible

        ColorLogic {
            id: colorLogic
            backendValue: spinBox.backendValue
            onValueFromBackendChanged: {
                if (colorLogic.valueFromBackend !== undefined)
                    spinBox.realValue = colorLogic.valueFromBackend
            }
        }

        labelColor: spinBox.edit ? StudioTheme.Values.themeTextColor : colorLogic.textColor

        onCompressedRealValueModified: spinBox.commitValue()
    }
}
