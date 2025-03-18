// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Layouts
import HelperWidgets
import StudioTheme as StudioTheme

Column {
    anchors.left: parent.left
    anchors.right: parent.right

    Section {
        caption: qsTr("Arc Item")

        anchors.left: parent.left
        anchors.right: parent.right

        SectionLayout {
            PropertyLabel {
                text: qsTr("Fill color")
                tooltip: qsTr("Sets the color inside the Arc stroke. This only works if the <b>Full outline</b> option is selected and the Arc stroke is not thick enough to fill the space.")
            }

            ColorEditor {
                backendValue: backendValues.fillColor
                supportGradient: true
                shapeGradients: true
                mcuGradients: true
            }

            PropertyLabel {
                text: qsTr("Stroke color")
                tooltip: qsTr("Sets the color of the Arc.")
            }

            ColorEditor {
                backendValue: backendValues.strokeColor
            }

            PropertyLabel {
                text: qsTr("Stroke width")
                tooltip: qsTr("Sets the thickness of the Arc.")
            }

            SecondColumnLayout {
                SpinBox {
                    id: strokeWidthSpinBox
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    backendValue: backendValues.strokeWidth
                    decimals: 1
                    minimumValue: -1
                    maximumValue: 200
                    stepSize: 1

                    property real previousValue: 0

                    onValueChanged: {
                        if (strokeWidthSpinBox.value > 0)
                            strokeWidthSpinBox.previousValue = strokeWidthSpinBox.value
                    }

                    Component.onCompleted: strokeWidthSpinBox.previousValue
                                           = Math.max(1, backendValues.strokeWidth.value)
                }

                Spacer {
                    implicitWidth: StudioTheme.Values.twoControlColumnGap
                                   + StudioTheme.Values.actionIndicatorWidth
                }

                CheckBox {
                    id: strokeWidthCheckBox
                    text: qsTr("Hide")
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                    checked: (backendValues.strokeWidth.value < 0)
                    actionIndicator.visible: false

                    onCheckedChanged: backendValues.strokeWidth.value
                                      = (strokeWidthCheckBox.checked ? -1 : strokeWidthSpinBox.previousValue)
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Arc start")
                tooltip: qsTr("Sets the start angle of the Arc.")
            }

            SecondColumnLayout {
                SpinBox {
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    backendValue: backendValues.begin
                    decimals: 1
                    minimumValue: -720
                    maximumValue: 720
                    stepSize: 1
                }

                Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

                ControlLabel { text: "°" }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Arc end")
                tooltip: qsTr("Sets the end angle of the Arc.")
            }

            SecondColumnLayout {
                SpinBox {
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    backendValue: backendValues.end
                    decimals: 1
                    minimumValue: -720
                    maximumValue: 720
                    stepSize: 1
                }

                Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

                ControlLabel { text: "°" }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Cap style")
                tooltip: qsTr("Sets the line ends as square or rounded.")
            }

            SecondColumnLayout {
                // copied from CapComboBox
                ComboBox {
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    width: implicitWidth
                    model: ["Flat Cap", "Square Cap", "Round Cap"]
                    backendValue: backendValues.capStyle
                    useInteger: true
                    manualMapping: true

                    property bool block: false

                    onValueFromBackendChanged: fromBackendToFrontend()

                    onCurrentTextChanged: {
                        if (!__isCompleted)
                            return

                        if (block)
                            return

                        if (currentText === "Flat Cap")
                            backendValues.capStyle.value = 0

                        if (currentText === "Square Cap")
                            backendValues.capStyle.value = 16

                        if (currentText === "Round Cap")
                            backendValues.capStyle.value = 32
                    }

                    Connections {
                        target: modelNodeBackend
                        function onSelectionChanged() { fromBackendToFrontend() }
                    }

                    function fromBackendToFrontend()
                    {
                        if (!__isCompleted || backendValues.capStyle === undefined)
                            return

                        block = true

                        if (backendValues.capStyle.value === 0)
                            currentIndex = 0
                        if (backendValues.capStyle.value === 16)
                            currentIndex = 1
                        if (backendValues.capStyle.value === 32)
                            currentIndex = 2

                        block = false
                    }
                }
            }

            ExpandingSpacer {}
        }
    }

    Section {
        caption: qsTr("Outline")

        anchors.left: parent.left
        anchors.right: parent.right

        SectionLayout {
            PropertyLabel {
                text: qsTr("Outline width")
                tooltip: qsTr("Sets the width of the outline that follows the Arc.")
            }

            SecondColumnLayout {
                SpinBox {
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    backendValue: backendValues.arcWidth
                    decimals: 1
                    minimumValue: 0
                    maximumValue: backendValues.maxArcWidth.value
                    stepSize: 1
                }

                Spacer { implicitWidth: StudioTheme.Values.twoControlColumnGap }

                CheckBox {
                    text: qsTr("Full outline")//backendValues.outlineArc.valueToString
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    backendValue: backendValues.outlineArc
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Round outline")
                tooltip: qsTr("Toggles the <b>Full outline</b> to have rounded edges.")
            }

            SecondColumnLayout {
                CheckBox {
                    text: backendValues.round.valueToString
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    backendValue: backendValues.round
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Round start")
                tooltip: qsTr("Toggles the starting edge of the <b>Full outline</b> to be rounded.")
            }

            SecondColumnLayout {
                CheckBox {
                    text: backendValues.roundBegin.valueToString
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    backendValue: backendValues.roundBegin
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Round end")
                tooltip: qsTr("Toggles the end edge of the <b>Full outline</b> to be rounded.")
            }

            SecondColumnLayout {
                CheckBox {
                    text: backendValues.roundEnd.valueToString
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    backendValue: backendValues.roundEnd
                }

                ExpandingSpacer {}
            }
        }
    }
}
