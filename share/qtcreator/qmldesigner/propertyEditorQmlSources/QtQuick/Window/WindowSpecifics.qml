// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import QtQuick.Layouts 1.15
import HelperWidgets 2.0
import StudioControls 1.0 as StudioControls
import StudioTheme 1.0 as StudioTheme

Section {
    anchors.left: parent.left
    anchors.right: parent.right
    caption: qsTr("Window")

    SectionLayout {
        PropertyLabel { text: qsTr("Title") }

        SecondColumnLayout {
            LineEdit {
                backendValue: backendValues.title
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                width: implicitWidth
            }

            ExpandingSpacer {}
        }

        PropertyLabel { text: qsTr("Position") }

        SecondColumnLayout {
            SpinBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.x
                maximumValue: 0xffff
                minimumValue: -0xffff
                decimals: 0
            }

            Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

            ControlLabel { text: "X" }

            Spacer { implicitWidth: StudioTheme.Values.controlGap }

            SpinBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.y
                maximumValue: 0xffff
                minimumValue: -0xffff
                decimals: 0
            }

            Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

            ControlLabel { text: "Y" }

            ExpandingSpacer {}
        }

        PropertyLabel { text: qsTr("Size") }

        SecondColumnLayout {
            SpinBox {
                backendValue: backendValues.width
                minimumValue: 0
                maximumValue: 10000
                decimals: 0
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

            ControlLabel {
                //: The width of the object
                text: qsTr("W", "width")
            }

            Spacer { implicitWidth: StudioTheme.Values.controlGap }

            SpinBox {
                backendValue: backendValues.height
                minimumValue: 0
                maximumValue: 10000
                decimals: 0
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
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
            tooltip: qsTr("Minimum size of the window.")
        }

        SecondColumnLayout {
            SpinBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.minimumWidth
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
                backendValue: backendValues.minimumHeight
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
            tooltip: qsTr("Maximum size of the window.")
        }

        SecondColumnLayout {
            SpinBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.maximumWidth
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
                backendValue: backendValues.maximumHeight
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

        PropertyLabel { text: qsTr("Color") }

        ColorEditor {
            backendValue: backendValues.color
            supportGradient: false
        }

        PropertyLabel { text: qsTr("Visible") }

        SecondColumnLayout {
            CheckBox {
                backendValue: backendValues.visible
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}
        }

        PropertyLabel { text: qsTr("Opacity") }

        SecondColumnLayout {
            SpinBox {
                backendValue: backendValues.opacity
                hasSlider: true
                minimumValue: 0
                maximumValue: 1
                stepSize: 0.1
                decimals: 2
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}
        }

        PropertyLabel { text: qsTr("Content orientation") }

        SecondColumnLayout {
            StudioControls.ComboBox {
                id: contentOrientationComboBox

                property bool __isCompleted: false

                property variant backendValue: backendValues.contentOrientation
                property variant valueFromBackend: contentOrientationComboBox.backendValue?.value ?? 0
                property bool isInModel: contentOrientationComboBox.backendValue?.isInModel ?? false
                property bool isInSubState: contentOrientationComboBox.backendValue?.isInSubState ?? false
                property bool block: false

                onIsInModelChanged: contentOrientationComboBox.evaluateValue()
                onIsInSubStateChanged: contentOrientationComboBox.evaluateValue()
                onBackendValueChanged: contentOrientationComboBox.evaluateValue()
                onValueFromBackendChanged: contentOrientationComboBox.evaluateValue()

                Connections {
                    target: modelNodeBackend
                    function onSelectionChanged() { contentOrientationComboBox.evaluateValue() }
                }

                function indexOfContentOrientation() {
                    if (contentOrientationComboBox.backendValue === undefined
                            || contentOrientationComboBox.backendValue.expression === undefined)
                        return 0

                    let value = contentOrientationComboBox.backendValue.expression

                    if (value.indexOf("PrimaryOrientation") !== -1) return 0
                    if (value.indexOf("LandscapeOrientation") !== -1) return 1
                    if (value.indexOf("PortraitOrientation") !== -1) return 2
                    if (value.indexOf("InvertedLandscapeOrientation") !== -1) return 3
                    if (value.indexOf("InvertedPortraitOrientation") !== -1) return 4

                    return 0
                }

                function evaluateValue() {
                    contentOrientationComboBox.block = true
                    contentOrientationComboBox.currentIndex = indexOfContentOrientation()
                    contentOrientationComboBox.block = false
                }

                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                width: implicitWidth
                labelColor: contentOrientationComboBox.currentIndex === 0
                                ? contentOrientationColorLogic.__defaultTextColor
                                : contentOrientationColorLogic.__changedTextColor
                model: ["PrimaryOrientation", "LandscapeOrientation", "PortraitOrientation",
                        "InvertedLandscapeOrientation", "InvertedPortraitOrientation"]

                ColorLogic { id: contentOrientationColorLogic }

                actionIndicator.icon.color: contentOrientationExtFuncLogic.color
                actionIndicator.icon.text: contentOrientationExtFuncLogic.glyph
                actionIndicator.onClicked: contentOrientationExtFuncLogic.show()
                actionIndicator.forceVisible: contentOrientationExtFuncLogic.menuVisible
                actionIndicator.visible: true

                ExtendedFunctionLogic {
                    id: contentOrientationExtFuncLogic
                    backendValue: backendValues.contentOrientation
                    onReseted: contentOrientationComboBox.currentIndex = 0
                }

                onActivated: function(index) {
                    if (!contentOrientationComboBox.__isCompleted)
                        return

                    contentOrientationComboBox.currentIndex = index
                    contentOrientationComboBox.composeExpressionString()
                }

                function composeExpressionString() {
                    if (contentOrientationComboBox.block)
                        return

                    var expressionStr = ""
                    if (contentOrientationComboBox.currentIndex !== 0) {
                        expressionStr = "Qt." + contentOrientationComboBox.currentText
                        contentOrientationComboBox.backendValue.expression = expressionStr
                    }
                }

                Component.onCompleted: {
                    contentOrientationComboBox.evaluateValue()
                    contentOrientationComboBox.__isCompleted = true
                }
            }

            ExpandingSpacer {}
        }

        PropertyLabel { text: qsTr("Flags") }

        SecondColumnLayout {
            StudioControls.ComboBox {
                id: flagsComboBox

                property bool __isCompleted: false

                property variant backendValue: backendValues.flags
                property variant valueFromBackend: flagsComboBox.backendValue?.value ?? 0
                property bool isInModel: flagsComboBox.backendValue?.isInModel ?? false
                property bool isInSubState: flagsComboBox.backendValue?.isInSubState ?? false
                property bool block: false

                onIsInModelChanged: flagsComboBox.evaluateValue()
                onIsInSubStateChanged: flagsComboBox.evaluateValue()
                onBackendValueChanged: flagsComboBox.evaluateValue()
                onValueFromBackendChanged: flagsComboBox.evaluateValue()

                Connections {
                    target: modelNodeBackend
                    function onSelectionChanged() { flagsComboBox.evaluateValue() }
                }

                function indexOfFlags() {
                    if (flagsComboBox.backendValue === undefined
                            || flagsComboBox.backendValue.expression === undefined)
                        return 0

                    let value = flagsComboBox.backendValue.expression

                    if (value.indexOf("Widget") !== -1) return 0
                    if (value.indexOf("Window") !== -1) return 1
                    if (value.indexOf("Dialog") !== -1) return 2
                    if (value.indexOf("Sheet") !== -1) return 3
                    if (value.indexOf("Drawer") !== -1) return 4
                    if (value.indexOf("Popup") !== -1) return 5
                    if (value.indexOf("Tool") !== -1) return 6
                    if (value.indexOf("ToolTip") !== -1) return 7
                    if (value.indexOf("SplashScreen") !== -1) return 8
                    if (value.indexOf("Desktop") !== -1) return 9
                    if (value.indexOf("SubWindow") !== -1) return 10
                    if (value.indexOf("ForeignWindow") !== -1) return 11
                    if (value.indexOf("CoverWindow") !== -1) return 12

                    return 0
                }

                function evaluateValue() {
                    flagsComboBox.block = true
                    flagsComboBox.currentIndex = indexOfFlags()
                    flagsComboBox.block = false
                }

                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                width: implicitWidth
                labelColor: flagsComboBox.currentIndex === 0 ? flagsColorLogic.__defaultTextColor
                                                             : flagsColorLogic.__changedTextColor
                model: ["Widget", "Window", "Dialog", "Sheet", "Drawer", "Popup", "Tool", "ToolTip",
                        "SplashScreen", "Desktop", "SubWindow", "ForeignWindow", "CoverWindow"]

                ColorLogic { id: flagsColorLogic }

                actionIndicator.icon.color: flagsExtFuncLogic.color
                actionIndicator.icon.text: flagsExtFuncLogic.glyph
                actionIndicator.onClicked: flagsExtFuncLogic.show()
                actionIndicator.forceVisible: flagsExtFuncLogic.menuVisible
                actionIndicator.visible: true

                ExtendedFunctionLogic {
                    id: flagsExtFuncLogic
                    backendValue: backendValues.flags
                    onReseted: flagsComboBox.currentIndex = 0
                }

                onActivated: function(index) {
                    if (!flagsComboBox.__isCompleted)
                        return

                    flagsComboBox.currentIndex = index
                    flagsComboBox.composeExpressionString()
                }

                function composeExpressionString() {
                    if (flagsComboBox.block)
                        return

                    var expressionStr = ""
                    if (flagsComboBox.currentIndex !== 0) {
                        expressionStr = "Qt." + flagsComboBox.currentText
                        flagsComboBox.backendValue.expression = expressionStr
                    }
                }

                Component.onCompleted: {
                    flagsComboBox.evaluateValue()
                    flagsComboBox.__isCompleted = true
                }
            }

            ExpandingSpacer {}
        }

        PropertyLabel { text: qsTr("Modality") }

        SecondColumnLayout {
            StudioControls.ComboBox {
                id: modalityComboBox

                property bool __isCompleted: false

                property variant backendValue: backendValues.modality
                property variant valueFromBackend: modalityComboBox.backendValue?.value ?? 0
                property bool isInModel: modalityComboBox.backendValue?.isInModel ?? false
                property bool isInSubState: modalityComboBox.backendValue?.isInSubState ?? false
                property bool block: false

                onIsInModelChanged: modalityComboBox.evaluateValue()
                onIsInSubStateChanged: modalityComboBox.evaluateValue()
                onBackendValueChanged: modalityComboBox.evaluateValue()
                onValueFromBackendChanged: modalityComboBox.evaluateValue()

                Connections {
                    target: modelNodeBackend
                    function onSelectionChanged() { modalityComboBox.evaluateValue() }
                }

                function indexOfModality() {
                    if (modalityComboBox.backendValue === undefined
                            || modalityComboBox.backendValue.expression === undefined)
                        return 0

                    let value = modalityComboBox.backendValue.expression

                    if (value.indexOf("NonModal") !== -1) return 0
                    if (value.indexOf("WindowModal") !== -1) return 1
                    if (value.indexOf("ApplicationModal") !== -1) return 2

                    return 0
                }

                function evaluateValue() {
                    modalityComboBox.block = true
                    modalityComboBox.currentIndex = indexOfModality()
                    modalityComboBox.block = false
                }

                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                width: implicitWidth
                labelColor: modalityComboBox.currentIndex === 0
                                ? modalityColorLogic.__defaultTextColor
                                : modalityColorLogic.__changedTextColor
                model: ["NonModal", "WindowModal", "ApplicationModal"]

                ColorLogic { id: modalityColorLogic }

                actionIndicator.icon.color: modalityExtFuncLogic.color
                actionIndicator.icon.text: modalityExtFuncLogic.glyph
                actionIndicator.onClicked: modalityExtFuncLogic.show()
                actionIndicator.forceVisible: modalityExtFuncLogic.menuVisible
                actionIndicator.visible: true

                ExtendedFunctionLogic {
                    id: modalityExtFuncLogic
                    backendValue: backendValues.modality
                    onReseted: modalityComboBox.currentIndex = 0
                }

                onActivated: function(index) {
                    if (!modalityComboBox.__isCompleted)
                        return

                    modalityComboBox.currentIndex = index
                    modalityComboBox.composeExpressionString()
                }

                function composeExpressionString() {
                    if (modalityComboBox.block)
                        return

                    var expressionStr = ""
                    if (modalityComboBox.currentIndex !== 0) {
                        expressionStr = "Qt." + modalityComboBox.currentText
                        modalityComboBox.backendValue.expression = expressionStr
                    }
                }

                Component.onCompleted: {
                    modalityComboBox.evaluateValue()
                    modalityComboBox.__isCompleted = true
                }
            }

            ExpandingSpacer {}
        }

        PropertyLabel { text: qsTr("Visibility") }

        SecondColumnLayout {
            ComboBox {
                scope: "Window"
                model: ["AutomaticVisibility", "Windowed", "Minimized", "Maximized", "FullScreen",
                        "Hidden"]
                backendValue: backendValues.visibility
                enabled: backendValues.visibility.isAvailable
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}
        }
    }
}
