/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

import QtQuick 2.1
import HelperWidgets 2.0
import QtQuick.Layouts 1.0
import StudioControls 1.0 as StudioControls
import QtQuickDesignerTheme 1.0

Section {
    id: fontSection
    anchors.left: parent.left
    anchors.right: parent.right
    caption: qsTr("Font")

    property string fontName: "font"

    property bool showStyle: false

    function getBackendValue(name)
    {
        return backendValues[fontSection.fontName + "_" + name]
    }

    property variant fontFamily: getBackendValue("family")
    property variant pointSize: getBackendValue("pointSize")
    property variant pixelSize: getBackendValue("pixelSize")

    property variant boldStyle: getBackendValue("bold")
    property variant italicStyle: getBackendValue("italic")
    property variant underlineStyle: getBackendValue("underline")
    property variant strikeoutStyle: getBackendValue("strikeout")

    onPointSizeChanged: {
        sizeWidget.setPointPixelSize();
    }

    onPixelSizeChanged: {
        sizeWidget.setPointPixelSize();
    }


    SectionLayout {
        columns: 2
        Label {
            text: qsTr("Font")
        }
        FontComboBox {
            id: fontComboBox
            backendValue: fontSection.fontFamily
            Layout.fillWidth: true
            width: 160
            property string familyName: backendValue.value
            onFamilyNameChanged: print(styleNamesForFamily(familyName))
        }

        Label {
            text: qsTr("Size")
        }

        RowLayout {
            id: sizeWidget
            property bool selectionFlag: selectionChanged

            property bool pixelSize: sizeType.currentText === "pixels"
            property bool isSetup;


            function setPointPixelSize() {
                sizeWidget.isSetup = true;
                sizeType.currentIndex = 1
                if (fontSection.pixelSize.isInModel)
                    sizeType.currentIndex = 0
                sizeWidget.isSetup = false;
            }

            onSelectionFlagChanged: {
                sizeWidget.setPointPixelSize();
            }

            Item {
                width: sizeSpinBox.width
                height: sizeSpinBox.height

                SpinBox {
                    id: sizeSpinBox
                    minimumValue: 0
                    visible: !sizeWidget.pixelSize
                    z: !sizeWidget.pixelSize ? 1 : 0
                    maximumValue: 400
                    backendValue: pointSize
                }

                SpinBox {
                    minimumValue: 0
                    visible: sizeWidget.pixelSize
                    z: sizeWidget.pixelSize ? 1 : 0
                    maximumValue: 400
                    backendValue: pixelSize
                }
            }

            StudioControls.ComboBox {
                id: sizeType
                model: ["pixels", "points"]
                property color textColor: Theme.color(Theme.PanelTextColorLight)
                actionIndicatorVisible: false

                onActivated: {
                    if (sizeWidget.isSetup)
                        return;
                    if (currentText == "pixels") {
                        pointSize.resetValue();
                        pixelSize.value = 8;
                    } else {
                        pixelSize.resetValue();
                    }

                }

                Layout.fillWidth: true
            }

        }

        Label {
            text: qsTr("Font style")
        }
        FontStyleButtons {

            bold: fontSection.boldStyle
            italic: fontSection.italicStyle
            underline: fontSection.underlineStyle
            strikeout: fontSection.strikeoutStyle
            enabled: !styleComboBox.styleSet
        }

        Label {
            text: qsTr("Font capitalization")
            toolTip: qsTr("Sets the capitalization for the text.")
        }

        ComboBox {
            Layout.fillWidth: true
            backendValue: getBackendValue("capitalization")
            model:  ["MixedCase", "AllUppercase", "AllLowercase", "SmallCaps", "Capitalize"]
            scope: "Font"
        }

        Label {
            text: qsTr("Font weight")
            toolTip: qsTr("Sets the font's weight.")
        }

        ComboBox {
            Layout.fillWidth: true
            backendValue: getBackendValue("weight")
            model:  ["Normal", "Light", "ExtraLight", "Thin", "Medium", "DemiBold", "Bold", "ExtraBold", "Black"]
            scope: "Font"
            enabled: !styleComboBox.styleSet
        }

        Label {
            text: qsTr("Style name")
            toolTip: qsTr("Sets the font's style.")
        }

        ComboBox {
            id: styleComboBox
            property bool styleSet: backendValue.isInModel
            Layout.fillWidth: true
            backendValue: getBackendValue("styleName")
            model: styleNamesForFamily(fontComboBox.familyName)
            valueType: ComboBox.String
        }

        Label {
            visible: showStyle
            text: qsTr("Style")
        }

        ComboBox {
            visible: showStyle
            Layout.fillWidth: true
            backendValue: (backendValues.style === undefined) ? dummyBackendValue : backendValues.style
            model:  ["Normal", "Outline", "Raised", "Sunken"]
            scope: "Text"
        }

        Label {
            text: qsTr("Spacing")
        }

        SecondColumnLayout {

            SpinBox {
                maximumValue: 500
                minimumValue: -500
                decimals: 2
                backendValue: getBackendValue("wordSpacing")
                Layout.fillWidth: true
                Layout.minimumWidth: 60
                stepSize: 0.1
            }
            Label {
                text: qsTr("Word")
                tooltip: qsTr("Sets the word spacing for the font.")
                width: 42
            }
            Item {
                width: 4
                height: 4
            }


            SpinBox {
                maximumValue: 500
                minimumValue: -500
                decimals: 2
                backendValue: getBackendValue("letterSpacing")
                Layout.fillWidth: true
                Layout.minimumWidth: 60
                stepSize: 0.1
            }
            Label {
                text: qsTr("Letter")
                tooltip: qsTr("Sets the letter spacing for the font.")
                width: 42
            }
        }

        Label {
            visible:  minorQtQuickVersion > 9
            text: qsTr("Performance")
        }

        SecondColumnLayout {
            visible:  minorQtQuickVersion > 9

            CheckBox {
                text: qsTr("Kerning")
                Layout.fillWidth: true
                backendValue: getBackendValue("kerning")
                tooltip: qsTr("Enables or disables the kerning OpenType feature when shaping the text. Disabling this may " +
                              "improve performance when creating or changing the text, at the expense of some cosmetic features. The default value is true.")
            }

            CheckBox {
                text: qsTr("Prefer shaping")
                Layout.fillWidth: true
                backendValue: getBackendValue("preferShaping")
                tooltip: qsTr("Sometimes, a font will apply complex rules to a set of characters in order to display them correctly.\n" +
                              "In some writing systems, such as Brahmic scripts, this is required in order for the text to be legible, whereas in " +
                              "Latin script,\n it is merely a cosmetic feature. Setting the preferShaping property to false will disable all such features\nwhen they are not required, which will improve performance in most cases.")
            }
        }

        Label {
            text: qsTr("Hinting preference")
            toolTip: qsTr("Sets the preferred hinting on the text.")
        }

        ComboBox {
            Layout.fillWidth: true
            backendValue: getBackendValue("hintingPreference")
            model: ["PreferDefaultHinting", "PreferNoHinting", "PreferVerticalHinting", "PreferFullHinting"]
            scope: "Font"
        }
    }
}
