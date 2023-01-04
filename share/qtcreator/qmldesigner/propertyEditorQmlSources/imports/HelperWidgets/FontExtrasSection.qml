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
    caption: qsTr("Font Extras")

    property string fontName: "font"
    property bool showStyle: false

    function getBackendValue(name) {
        return backendValues[root.fontName + "_" + name]
    }

    function isBackendValueAvailable(name) {
        if (backendValues[name] !== undefined)
            return backendValues[name].isAvailable

        return false
    }

    SectionLayout {
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
            visible: root.showStyle
            text: qsTr("Style")
            blockedByTemplate: !styleComboBox.enabled
        }

        SecondColumnLayout {
            visible: root.showStyle

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
            visible: root.isBackendValueAvailable("styleColor")
        }

        ColorEditor {
            visible: root.isBackendValueAvailable("styleColor")
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
