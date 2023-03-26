// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import QtQuick.Layouts 1.15
import HelperWidgets 2.0
import StudioControls 1.0 as StudioControls
import StudioTheme 1.0 as StudioTheme

Section {
    id: fontSection
    anchors.left: parent.left
    anchors.right: parent.right
    caption: qsTr("Font")

    property string fontName: "font"

    property bool showStyle: false

    function getBackendValue(name) {
        return backendValues[fontSection.fontName + "_" + name]
    }

    property variant fontFamily: getBackendValue("family")
    property variant pointSize: getBackendValue("pointSize")
    property variant pixelSize: getBackendValue("pixelSize")

    property variant boldStyle: getBackendValue("bold")
    property variant italicStyle: getBackendValue("italic")
    property variant underlineStyle: getBackendValue("underline")
    property variant strikeoutStyle: getBackendValue("strikeout")

    onPointSizeChanged: sizeWidget.setPointPixelSize()
    onPixelSizeChanged: sizeWidget.setPointPixelSize()

    SectionLayout {
        PropertyLabel {
            text: qsTr("Font")
            tooltip: qsTr("Sets the font of the text.")
        }

        SecondColumnLayout {
            FontComboBox {
                id: fontComboBox
                property string familyName: backendValue.value
                backendValue: fontSection.fontFamily
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                width: implicitWidth
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Size")
            tooltip: qsTr("Sets the font size in pixels or points.")
        }

        SecondColumnLayout {
            id: sizeWidget
            property bool selectionFlag: selectionChanged

            property bool pixelSize: sizeType.currentText === "px"
            property bool isSetup

            function setPointPixelSize() {
                sizeWidget.isSetup = true
                sizeType.currentIndex = 1

                if (fontSection.pixelSize.isInModel)
                    sizeType.currentIndex = 0

                sizeWidget.isSetup = false
            }

            onSelectionFlagChanged: sizeWidget.setPointPixelSize()

            Item {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                width: implicitWidth
                height: sizeSpinBox.height

                SpinBox {
                    id: sizeSpinBox
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    width: implicitWidth
                    minimumValue: 0
                    visible: !sizeWidget.pixelSize
                    z: !sizeWidget.pixelSize ? 1 : 0
                    maximumValue: 400
                    backendValue: pointSize
                }
                SpinBox {
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    width: implicitWidth
                    minimumValue: 0
                    visible: sizeWidget.pixelSize
                    z: sizeWidget.pixelSize ? 1 : 0
                    maximumValue: 400
                    backendValue: pixelSize
                }
            }

            Spacer {
                implicitWidth: StudioTheme.Values.twoControlColumnGap
                               + StudioTheme.Values.actionIndicatorWidth
            }

            StudioControls.ComboBox {
                id: sizeType
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                width: implicitWidth
                model: ["px", "pt"]
                actionIndicatorVisible: false

                onActivated: {
                    if (sizeWidget.isSetup)
                        return

                    if (sizeType.currentText === "px") {
                        pointSize.resetValue()
                        pixelSize.value = 8
                    } else {
                        pixelSize.resetValue()
                    }
                }
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Emphasis")
            tooltip: qsTr("Sets the text to bold, italic, underlined, or strikethrough.")
            blockedByTemplate: !fontSection.boldStyle.isAvailable
                               && !fontSection.italicStyle.isAvailable
                               && !fontSection.underlineStyle.isAvailable
                               && !fontSection.strikeoutStyle.isAvailable
        }

        FontStyleButtons {
            bold: fontSection.boldStyle
            italic: fontSection.italicStyle
            underline: fontSection.underlineStyle
            strikeout: fontSection.strikeoutStyle
            enabled: !styleNameComboBox.styleSet
        }

        PropertyLabel {
            text: qsTr("Capitalization")
            tooltip: qsTr("Sets capitalization rules for the text.")
            blockedByTemplate: !getBackendValue("capitalization").isAvailable
        }

        SecondColumnLayout {
            ComboBox {
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                width: implicitWidth
                backendValue: getBackendValue("capitalization")
                scope: "Font"
                model: ["MixedCase", "AllUppercase", "AllLowercase", "SmallCaps", "Capitalize"]
                enabled: backendValue.isAvailable
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Weight")
            tooltip: qsTr("Sets the overall thickness of the font.")
            blockedByTemplate: styleNameComboBox.styleSet
        }

        SecondColumnLayout {
            ComboBox {
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                width: implicitWidth
                backendValue: getBackendValue("weight")
                model: ["Normal", "Light", "ExtraLight", "Thin", "Medium", "DemiBold", "Bold", "ExtraBold", "Black"]
                scope: "Font"
                enabled: !styleNameComboBox.styleSet
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Style name")
            tooltip: qsTr("Sets the style of the selected font. This is prioritized over <b>Weight</b> and <b>Emphasis</b>.")
            blockedByTemplate: !styleNameComboBox.enabled
        }

        SecondColumnLayout {
            ComboBox {
                id: styleNameComboBox
                property bool styleSet: backendValue.isInModel
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                width: implicitWidth
                backendValue: getBackendValue("styleName")
                model: styleNamesForFamily(fontComboBox.familyName)
                valueType: ComboBox.String
                enabled: backendValue.isAvailable
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            visible: fontSection.showStyle
            text: qsTr("Style")
            tooltip: qsTr("Sets the font style.")
            blockedByTemplate: !styleComboBox.enabled
        }

        SecondColumnLayout {
            visible: fontSection.showStyle
            ComboBox {
                id: styleComboBox
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                width: implicitWidth
                backendValue: (backendValues.style === undefined) ? dummyBackendValue
                                                                  : backendValues.style
                scope: "Text"
                model: ["Normal", "Outline", "Raised", "Sunken"]
                enabled: backendValue.isAvailable
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Style color")
            tooltip: qsTr("Sets the color for the font style.")
            visible: fontSection.showStyle && backendValues.styleColor.isAvailable
        }

        ColorEditor {
            visible: fontSection.showStyle && backendValues.styleColor.isAvailable
            backendValue: backendValues.styleColor
            supportGradient: false
        }

        PropertyLabel {
            text: qsTr("Hinting")
            tooltip: qsTr("Sets how to interpolate the text to render it more clearly when scaled.")
            blockedByTemplate: !getBackendValue("hintingPreference").isAvailable
        }

        SecondColumnLayout {
            ComboBox {
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                width: implicitWidth
                backendValue: getBackendValue("hintingPreference")
                scope: "Font"
                model: ["PreferDefaultHinting", "PreferNoHinting", "PreferVerticalHinting", "PreferFullHinting"]
                enabled: backendValue.isAvailable
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Letter spacing")
            tooltip: qsTr("Sets the letter spacing for the text.")
            blockedByTemplate: !getBackendValue("letterSpacing").isAvailable
        }

        SecondColumnLayout {
            SpinBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: getBackendValue("letterSpacing")
                decimals: 2
                minimumValue: -500
                maximumValue: 500
                stepSize: 0.1
                enabled: backendValue.isAvailable
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Word spacing")
            tooltip: qsTr("Sets the word spacing for the text.")
            blockedByTemplate: !getBackendValue("wordSpacing").isAvailable
        }

        SecondColumnLayout {
            SpinBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: getBackendValue("wordSpacing")
                decimals: 2
                minimumValue: -500
                maximumValue: 500
                stepSize: 0.1
                enabled: backendValue.isAvailable
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Auto kerning")
            tooltip: qsTr("Resolves the gap between texts if turned true.")
            blockedByTemplate: !getBackendValue("kerning").isAvailable
        }

        SecondColumnLayout {
            CheckBox {
                text: backendValue.valueToString
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: getBackendValue("kerning")
                enabled: backendValue.isAvailable
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Prefer shaping")
            tooltip: qsTr("Toggles the disables font-specific special features.")
            blockedByTemplate: !getBackendValue("preferShaping").isAvailable
        }

        SecondColumnLayout {
            CheckBox {
                text: backendValue.valueToString
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: getBackendValue("preferShaping")
                enabled: backendValue.isAvailable
            }

            ExpandingSpacer {}
        }
    }
}
