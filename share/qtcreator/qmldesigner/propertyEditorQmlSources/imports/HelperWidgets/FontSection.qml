/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

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
        PropertyLabel { text: qsTr("Font") }

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

        PropertyLabel { text: qsTr("Size") }

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
            tooltip: qsTr("Capitalization for the text.")
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
            tooltip: qsTr("Font's weight.")
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
            tooltip: qsTr("Font's style.")
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
            visible: fontSection.showStyle && backendValues.styleColor.isAvailable
        }

        ColorEditor {
            visible: fontSection.showStyle && backendValues.styleColor.isAvailable
            backendValue: backendValues.styleColor
            supportGradient: false
        }

        PropertyLabel {
            text: qsTr("Hinting")
            tooltip: qsTr("Preferred hinting on the text.")
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
            tooltip: qsTr("Letter spacing for the font.")
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
            tooltip: qsTr("Word spacing for the font.")
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
            tooltip: qsTr("Enables or disables the kerning OpenType feature when shaping the text. Disabling this may " +
                          "improve performance when creating or changing the text, at the expense of some cosmetic features.")
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
            tooltip: qsTr("Sometimes, a font will apply complex rules to a set of characters in order to display them correctly.\n" +
                          "In some writing systems, such as Brahmic scripts, this is required in order for the text to be legible, whereas in " +
                          "Latin script,\n it is merely a cosmetic feature. Setting the preferShaping property to false will disable all such features\nwhen they are not required, which will improve performance in most cases.")
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
