// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import QtQuick.Layouts 1.15
import HelperWidgets 2.0
import StudioControls 1.0 as StudioControls
import StudioTheme 1.0 as StudioTheme

Section {
    id: root
    anchors.left: parent.left
    anchors.right: parent.right
    caption: qsTr("Font")

    property string fontName: "font"

    property bool showStyle: false
    property bool showHinting: true

    function getBackendValue(name) {
        return backendValues[root.fontName + "_" + name]
    }

    property variant fontFamily: root.getBackendValue("family")
    property variant pointSize: root.getBackendValue("pointSize")
    property variant pixelSize: root.getBackendValue("pixelSize")

    property variant boldStyle: root.getBackendValue("bold")
    property variant italicStyle: root.getBackendValue("italic")
    property variant underlineStyle: root.getBackendValue("underline")
    property variant strikeoutStyle: root.getBackendValue("strikeout")

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

                property string familyName: fontComboBox.backendValue.value

                backendValue: root.fontFamily
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                width: fontComboBox.implicitWidth
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

                if (root.pixelSize.isInModel)
                    sizeType.currentIndex = 0

                sizeWidget.isSetup = false
            }

            onSelectionFlagChanged: sizeWidget.setPointPixelSize()

            Item {
                id: sizeSpinBoxItem
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                width: sizeSpinBoxItem.implicitWidth
                height: sizeSpinBoxPoint.height

                SpinBox {
                    id: sizeSpinBoxPoint
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    width: sizeSpinBoxPoint.implicitWidth
                    minimumValue: 0
                    visible: !sizeWidget.pixelSize
                    z: !sizeWidget.pixelSize ? 1 : 0
                    maximumValue: 400
                    backendValue: root.pointSize
                }
                SpinBox {
                    id: sizeSpinBoxPixel
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    width: sizeSpinBoxPixel.implicitWidth
                    minimumValue: 0
                    visible: sizeWidget.pixelSize
                    z: sizeWidget.pixelSize ? 1 : 0
                    maximumValue: 400
                    backendValue: root.pixelSize
                }
            }

            Spacer {
                implicitWidth: StudioTheme.Values.twoControlColumnGap
                               + StudioTheme.Values.actionIndicatorWidth
            }

            StudioControls.ComboBox {
                id: sizeType
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                width: sizeType.implicitWidth
                model: ["px", "pt"]
                actionIndicatorVisible: false

                onActivated: {
                    if (sizeWidget.isSetup)
                        return

                    if (sizeType.currentText === "px") {
                        root.pointSize.resetValue()
                        root.pixelSize.value = 8
                    } else {
                        root.pixelSize.resetValue()
                    }
                }
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Emphasis")
            tooltip: qsTr("Sets the text to bold, italic, underlined, or strikethrough.")
            blockedByTemplate: !root.boldStyle.isAvailable
                               && !root.italicStyle.isAvailable
                               && !root.underlineStyle.isAvailable
                               && !root.strikeoutStyle.isAvailable
        }

        FontStyleButtons {
            bold: root.boldStyle
            italic: root.italicStyle
            underline: root.underlineStyle
            strikeout: root.strikeoutStyle
            enabled: !styleNameComboBox.styleSet
        }

        PropertyLabel {
            text: qsTr("Capitalization")
            tooltip: qsTr("Sets capitalization rules for the text.")
            blockedByTemplate: !root.getBackendValue("capitalization").isAvailable
        }

        SecondColumnLayout {
            ComboBox {
                id: capitalizationComboBox
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                width: capitalizationComboBox.implicitWidth
                backendValue: root.getBackendValue("capitalization")
                scope: "Font"
                model: ["MixedCase", "AllUppercase", "AllLowercase", "SmallCaps", "Capitalize"]
                enabled: capitalizationComboBox.backendValue.isAvailable
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
                id: weightComboBox
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                width: weightComboBox.implicitWidth
                backendValue: root.getBackendValue("weight")
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
                width: styleNameComboBox.implicitWidth
                backendValue: root.getBackendValue("styleName")
                model: styleNamesForFamily(fontComboBox.familyName)
                valueType: ComboBox.String
                enabled: styleNameComboBox.backendValue.isAvailable
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            visible: root.showStyle
            text: qsTr("Style")
            tooltip: qsTr("Sets the font style.")
            blockedByTemplate: !styleComboBox.enabled
        }

        SecondColumnLayout {
            visible: root.showStyle

            ComboBox {
                id: styleComboBox
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                width: styleComboBox.implicitWidth
                backendValue: (backendValues.style === undefined) ? dummyBackendValue
                                                                  : backendValues.style
                scope: "Text"
                model: ["Normal", "Outline", "Raised", "Sunken"]
                enabled: styleComboBox.backendValue.isAvailable
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Style color")
            tooltip: qsTr("Sets the color for the font style.")
            visible: root.showStyle && backendValues.styleColor.isAvailable
        }

        ColorEditor {
            visible: root.showStyle && backendValues.styleColor.isAvailable
            backendValue: backendValues.styleColor
            supportGradient: false
        }

        PropertyLabel {
            visible: root.showHinting
            text: qsTr("Hinting")
            tooltip: qsTr("Sets how to interpolate the text to render it more clearly when scaled.")
            blockedByTemplate: !root.getBackendValue("hintingPreference").isAvailable
        }

        SecondColumnLayout {
            visible: root.showHinting

            ComboBox {
                id: hintingPreferenceComboBox
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                width: hintingPreferenceComboBox.implicitWidth
                backendValue: root.getBackendValue("hintingPreference")
                scope: "Font"
                model: ["PreferDefaultHinting", "PreferNoHinting", "PreferVerticalHinting", "PreferFullHinting"]
                enabled: hintingPreferenceComboBox.backendValue.isAvailable
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Letter spacing")
            tooltip: qsTr("Sets the letter spacing for the text.")
            blockedByTemplate: !root.getBackendValue("letterSpacing").isAvailable
        }

        SecondColumnLayout {
            SpinBox {
                id: letterSpacingSpinBox
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: root.getBackendValue("letterSpacing")
                decimals: 2
                minimumValue: -500
                maximumValue: 500
                stepSize: 0.1
                enabled: letterSpacingSpinBox.backendValue.isAvailable
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Word spacing")
            tooltip: qsTr("Sets the word spacing for the text.")
            blockedByTemplate: !root.getBackendValue("wordSpacing").isAvailable
        }

        SecondColumnLayout {
            SpinBox {
                id: wordSpacingSpinBox
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: root.getBackendValue("wordSpacing")
                decimals: 2
                minimumValue: -500
                maximumValue: 500
                stepSize: 0.1
                enabled: wordSpacingSpinBox.backendValue.isAvailable
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Auto kerning")
            tooltip: qsTr("Resolves the gap between texts if turned true.")
            blockedByTemplate: !root.getBackendValue("kerning").isAvailable
        }

        SecondColumnLayout {
            CheckBox {
                id: kerningCheckBox
                text: kerningCheckBox.backendValue.valueToString
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: root.getBackendValue("kerning")
                enabled: kerningCheckBox.backendValue.isAvailable
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Prefer shaping")
            tooltip: qsTr("Toggles the disables font-specific special features.")
            blockedByTemplate: !root.getBackendValue("preferShaping").isAvailable
        }

        SecondColumnLayout {
            CheckBox {
                id: preferShapingCheckBox
                text: preferShapingCheckBox.backendValue.valueToString
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: root.getBackendValue("preferShaping")
                enabled: preferShapingCheckBox.backendValue.isAvailable
            }

            ExpandingSpacer {}
        }
    }
}
