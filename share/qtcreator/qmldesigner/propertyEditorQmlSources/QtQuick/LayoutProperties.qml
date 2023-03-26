// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import QtQuick.Layouts 1.15
import HelperWidgets 2.0
import StudioControls 1.0 as StudioControls
import StudioTheme 1.0 as StudioTheme

SectionLayout {
    id: root

    property variant backendValue: backendValues.Layout_alignment
    property variant valueFromBackend: backendValue === undefined ? 0 : backendValue.value
    property bool isInModel: backendValue === undefined ? false : backendValue.isInModel
    property bool isInSubState: backendValue === undefined ? false : backendValue.isInSubState
    property bool blockAlignment: false

    onIsInModelChanged: evaluateAlignment()
    onIsInSubStateChanged: evaluateAlignment()
    onBackendValueChanged: evaluateAlignment()
    onValueFromBackendChanged: evaluateAlignment()

    Connections {
        target: modelNodeBackend
        function onSelectionChanged() {
            evaluateAlignment()
        }
    }

    Component.onCompleted: evaluateAlignment()

    function indexOfVerticalAlignment() {
        if (backendValue === undefined)
            return 0

        if (backendValue.expression === undefined)
            return 0

        if (backendValue.expression.indexOf("AlignVCenter") !== -1)
            return 0

        if (backendValue.expression.indexOf("AlignTop") !== -1)
            return 1

        if (backendValue.expression.indexOf("AlignBottom") !== -1)
            return 2

        if (backendValue.expression.indexOf("AlignBaseline") !== -1)
            return 3

        return 0
    }

    function indexOfHorizontalAlignment() {
        if (backendValue === undefined)
            return 0

        if (backendValue.expression === undefined)
            return 0

        if (backendValue.expression.indexOf("AlignLeft") !== -1)
            return 0

        if (backendValue.expression.indexOf("AlignHCenter") !== -1)
            return 1

        if (backendValue.expression.indexOf("AlignRight") !== -1)
            return 2

        return 0
    }

    function evaluateAlignment() {
        blockAlignment = true

        verticalAlignmentComboBox.currentIndex = indexOfVerticalAlignment()
        horizontalAlignmentComboBox.currentIndex = indexOfHorizontalAlignment()

        blockAlignment = false
    }

    function composeExpressionString() {
        if (blockAlignment)
            return

        var expressionStr = "";
        if (horizontalAlignmentComboBox.currentIndex !== 0
                || verticalAlignmentComboBox.currentIndex !== 0) {
            expressionStr = "Qt." + horizontalAlignmentComboBox.currentText
                    + " | "
                    + "Qt." + verticalAlignmentComboBox.currentText

            backendValue.expression = expressionStr
        } else {
            expressionStr = "Qt.AlignLeft | Qt.AlignVCenter";
            backendValue.expression = expressionStr
            backendValue.resetValue()
        }
    }

    PropertyLabel {
        text: qsTr("Alignment")
        tooltip: qsTr("Alignment of a component within the cells it occupies.")
    }

    SecondColumnLayout {
        StudioControls.ComboBox {
            id: horizontalAlignmentComboBox

            property bool __isCompleted: false

            implicitWidth: StudioTheme.Values.singleControlColumnWidth
                           + StudioTheme.Values.actionIndicatorWidth
            labelColor: horizontalAlignmentComboBox.currentIndex === 0 ? colorLogic.__defaultTextColor
                                                                       : colorLogic.__changedTextColor
            model: ["AlignLeft", "AlignHCenter", "AlignRight"]

            actionIndicator.icon.color: extFuncLogic.color
            actionIndicator.icon.text: extFuncLogic.glyph
            actionIndicator.onClicked: extFuncLogic.show()
            actionIndicator.forceVisible: extFuncLogic.menuVisible
            actionIndicator.visible: true

            onActivated: {
                if (!horizontalAlignmentComboBox.__isCompleted)
                    return

                horizontalAlignmentComboBox.currentIndex = index
                composeExpressionString()
            }

            Component.onCompleted: horizontalAlignmentComboBox.__isCompleted = true

            ColorLogic { id: colorLogic }

            ExtendedFunctionLogic {
                id: extFuncLogic
                backendValue: backendValues.Layout_alignment
                onReseted: {
                    horizontalAlignmentComboBox.currentIndex = 0
                    verticalAlignmentComboBox.currentIndex = 0
                }
            }
        }
    }

    PropertyLabel { text: "" }

    SecondColumnLayout {
        StudioControls.ComboBox {
            id: verticalAlignmentComboBox

            property bool __isCompleted: false

            implicitWidth: StudioTheme.Values.singleControlColumnWidth
                           + StudioTheme.Values.actionIndicatorWidth
            labelColor: verticalAlignmentComboBox.currentIndex === 0 ? colorLogic.__defaultTextColor
                                                                     : colorLogic.__changedTextColor
            model: ["AlignVCenter", "AlignTop", "AlignBottom", "AlignBaseline"]

            actionIndicator.icon.color: extFuncLogic.color
            actionIndicator.icon.text: extFuncLogic.glyph
            actionIndicator.onClicked: extFuncLogic.show()
            actionIndicator.forceVisible: extFuncLogic.menuVisible
            actionIndicator.visible: true

            onActivated: {
                if (!verticalAlignmentComboBox.__isCompleted)
                    return

                verticalAlignmentComboBox.currentIndex = index
                composeExpressionString()
            }

            Component.onCompleted: verticalAlignmentComboBox.__isCompleted = true
        }
    }

    PropertyLabel {
        text: qsTr("Fill layout")
        tooltip: qsTr("Expands the component as much as possible within the given constraints.")
    }

    SecondColumnLayout {
        CheckBox {
            implicitWidth: StudioTheme.Values.twoControlColumnWidth
                           + StudioTheme.Values.actionIndicatorWidth
            backendValue: backendValues.Layout_fillWidth
            text: qsTr("Width")
        }

        Spacer {
            implicitWidth: StudioTheme.Values.twoControlColumnGap
        }

        CheckBox {
            implicitWidth: StudioTheme.Values.twoControlColumnWidth
                           + StudioTheme.Values.actionIndicatorWidth
            backendValue: backendValues.Layout_fillHeight
            text: qsTr("Height")
        }
    }

    PropertyLabel {
        text: qsTr("Preferred size")
        tooltip: qsTr("Preferred size of a component in a layout. If the preferred height or width is -1, it is ignored.")
    }

    SecondColumnLayout {
        SpinBox {
            implicitWidth: StudioTheme.Values.twoControlColumnWidth
                           + StudioTheme.Values.actionIndicatorWidth
            backendValue: backendValues.Layout_preferredWidth
            minimumValue: -1
            maximumValue: 0xffff
            decimals: 0
        }

        Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

        ControlLabel {
            //: The width of the object
            text: qsTr("W", "width")
        }

        Spacer { implicitWidth: StudioTheme.Values.controlGap }

        SpinBox {
            implicitWidth: StudioTheme.Values.twoControlColumnWidth
                           + StudioTheme.Values.actionIndicatorWidth
            backendValue: backendValues.Layout_preferredHeight
            minimumValue: -1
            maximumValue: 0xffff
            decimals: 0
        }

        Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

        ControlLabel {
            //: The height of the object
            text: qsTr("H", "height")
        }

        ExpandingSpacer {}
    }

    PropertyLabel {
        text: qsTr("Minimum size")
        tooltip: qsTr("Minimum size of a component in a layout.")
    }

    SecondColumnLayout {
        SpinBox {
            implicitWidth: StudioTheme.Values.twoControlColumnWidth
                           + StudioTheme.Values.actionIndicatorWidth
            backendValue: backendValues.Layout_minimumWidth
            minimumValue: 0
            maximumValue: 0xffff
            decimals: 0
        }

        Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

        ControlLabel {
            //: The width of the object
            text: qsTr("W", "width")
        }

        Spacer { implicitWidth: StudioTheme.Values.controlGap }

        SpinBox {
            implicitWidth: StudioTheme.Values.twoControlColumnWidth
                           + StudioTheme.Values.actionIndicatorWidth
            backendValue: backendValues.Layout_minimumHeight
            minimumValue: 0
            maximumValue: 0xffff
            decimals: 0
        }

        Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

        ControlLabel {
            //: The height of the object
            text: qsTr("H", "height")
        }

        ExpandingSpacer {}
    }

    PropertyLabel {
        text: qsTr("Maximum size")
        tooltip: qsTr("Maximum size of a component in a layout.")
    }

    SecondColumnLayout {
        SpinBox {
            implicitWidth: StudioTheme.Values.twoControlColumnWidth
                           + StudioTheme.Values.actionIndicatorWidth
            backendValue: backendValues.Layout_maximumWidth
            minimumValue: 0
            maximumValue: 0xffff
            decimals: 0
        }

        Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

        ControlLabel {
            //: The width of the object
            text: qsTr("W", "width")
        }

        Spacer { implicitWidth: StudioTheme.Values.controlGap }

        SpinBox {
            implicitWidth: StudioTheme.Values.twoControlColumnWidth
                           + StudioTheme.Values.actionIndicatorWidth
            backendValue: backendValues.Layout_maximumHeight
            minimumValue: 0
            maximumValue: 0xffff
            decimals: 0
        }

        Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

        ControlLabel {
            //: The height of the object
            text: qsTr("H", "height")
        }

        ExpandingSpacer {}
    }

    PropertyLabel {
        text: qsTr("Row span")
        tooltip: qsTr("Row span of a component in a Grid Layout.")
    }

    SecondColumnLayout {
        SpinBox {
            implicitWidth: StudioTheme.Values.twoControlColumnWidth
                           + StudioTheme.Values.actionIndicatorWidth
            backendValue: backendValues.Layout_rowSpan
            minimumValue: 0
            maximumValue: 0xffff
            decimals: 0
        }

        ExpandingSpacer {}
    }

    PropertyLabel {
        text: qsTr("Column span")
        tooltip: qsTr("Column span of a component in a Grid Layout.")
    }

    SecondColumnLayout {
        SpinBox {
            implicitWidth: StudioTheme.Values.twoControlColumnWidth
                           + StudioTheme.Values.actionIndicatorWidth
            backendValue: backendValues.Layout_columnSpan
            minimumValue: 0
            maximumValue: 0xffff
            decimals: 0
        }

        ExpandingSpacer {}
    }
}
