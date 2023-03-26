// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import QtQuick.Layouts 1.15
import HelperWidgets 2.0
import StudioTheme 1.0 as StudioTheme

Section {
    id: root

    property bool blockedByContext: backendValues.display.enumeration === "TextOnly"

    caption: qsTr("Icon")
    width: parent.width

    SectionLayout {
        // We deliberately kept the "name" property out as it is only properly supported by linux
        // based operating systems out of the box.

        PropertyLabel {
            text: qsTr("Source")
            tooltip: qsTr("Sets a background image for the icon.")
            blockedByTemplate: !backendValues.icon_source.isAvailable
            enabled: !root.blockedByContext
        }

        SecondColumnLayout {
            UrlChooser {
                backendValue: backendValues.icon_source
                enabled: backendValues.icon_source.isAvailable && !root.blockedByContext
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Color")
            tooltip: qsTr("Sets the color for the icon.")
            blockedByTemplate: !backendValues.icon_color.isAvailable
            enabled: !root.blockedByContext
        }

        ColorEditor {
            backendValue: backendValues.icon_color
            supportGradient: false
            enabled: backendValues.icon_color.isAvailable && !root.blockedByContext
        }

        PropertyLabel {
            text: qsTr("Size")
            tooltip: qsTr("Sets the height and width of the icon.")
            blockedByTemplate: !backendValues.icon_width.isAvailable
            enabled: !root.blockedByContext
        }

        SecondColumnLayout {
            SpinBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.icon_width
                maximumValue: 0xffff
                minimumValue: 0
                decimals: 0
                enabled: backendValues.icon_width.isAvailable && !root.blockedByContext
            }

            Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

            ControlLabel {
                //: The width of the object
                text: qsTr("W", "width")
                tooltip: qsTr("Width")
                enabled: backendValues.icon_width.isAvailable && !root.blockedByContext
            }

            Spacer { implicitWidth: StudioTheme.Values.controlGap }

            SpinBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.icon_height
                maximumValue: 0xffff
                minimumValue: 0
                decimals: 0
                enabled: backendValues.icon_height.isAvailable && !root.blockedByContext
            }

            Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

            ControlLabel {
                //: The height of the object
                text: qsTr("H", "height")
                tooltip: qsTr("Height")
                enabled: backendValues.icon_height.isAvailable && !root.blockedByContext
            }
/*
            TODO QDS-4836
            Spacer { implicitWidth: StudioTheme.Values.controlGap }

            LinkIndicator2D {}
*/
            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Cache")
            tooltip: qsTr("Toggles if the icon is saved to the cache memory.")
            blockedByTemplate: !backendValues.icon_cache.isAvailable
            enabled: !root.blockedByContext
        }

        SecondColumnLayout {
            CheckBox {
                text: backendValues.icon_cache.valueToString
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.icon_cache
                enabled: backendValues.icon_cache.isAvailable && !root.blockedByContext
            }

            ExpandingSpacer {}
        }

    }
}
