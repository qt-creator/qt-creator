// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Layouts
import HelperWidgets
import StudioTheme as StudioTheme

Section {
    caption: qsTr("Button Content")

    anchors.left: parent.left
    anchors.right: parent.right

    SectionLayout {
        PropertyLabel {
            text: qsTr("Text")
            tooltip: qsTr("Text displayed on the button.")
        }

        SecondColumnLayout {
            LineEdit {
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                width: implicitWidth
                backendValue: backendValues.text
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Display")
            tooltip: qsTr("Determines how the icon and text are displayed within the button.")
            blockedByTemplate: !backendValues.display.isAvailable
        }

        SecondColumnLayout {
            ComboBox {
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                width: implicitWidth
                backendValue: backendValues.display
                model: [ "IconOnly", "TextOnly", "TextBesideIcon", "TextUnderIcon" ]
                scope: "AbstractButton"
                enabled: backendValue.isAvailable
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Checkable")
            tooltip: qsTr("Toggles if the button is checkable.")
        }

        SecondColumnLayout {
            CheckBox {
                text: backendValues.checkable.valueToString
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.checkable
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Checked")
            tooltip: qsTr("Toggles if the button is checked.")
        }

        SecondColumnLayout {
            CheckBox {
                text: backendValues.checked.valueToString
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.checked
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Exclusive")
            tooltip: qsTr("Toggles if the button is exclusive. Non-exclusive checkable buttons that belong to the same parent behave as if they are part of the same button group; only one button can be checked at any time.")
            blockedByTemplate: !backendValues.autoExclusive.isAvailable
        }

        SecondColumnLayout {
            CheckBox {
                text: backendValues.autoExclusive.valueToString
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.autoExclusive
                enabled: backendValue.isAvailable
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Auto-repeat")
            tooltip: qsTr("Toggles if pressed, released, and clicked actions are repeated while the button is pressed and held down.")
        }

        SecondColumnLayout {
            CheckBox {
                id: autoRepeat
                text: backendValues.autoRepeat.valueToString
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.autoRepeat
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Repeat delay")
            tooltip: qsTr("Sets the initial delay of auto-repetition in milliseconds.")
            enabled: autoRepeat.checked
        }

        SecondColumnLayout {
            SpinBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                width: implicitWidth
                minimumValue: 0
                maximumValue: 9999999
                decimals: 0
                backendValue: backendValues.autoRepeatDelay
                enabled: autoRepeat.checked
            }

            Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

            ControlLabel {
                text: "ms"
                elide: Text.ElideNone
                enabled: autoRepeat.checked
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Repeat interval")
            tooltip: qsTr("Sets the interval between auto-repetitions in milliseconds.")
            enabled: autoRepeat.checked
        }

        SecondColumnLayout {
            SpinBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                width: implicitWidth
                minimumValue: 0
                maximumValue: 9999999
                decimals: 0
                backendValue: backendValues.autoRepeatInterval
                enabled: autoRepeat.checked
            }

            Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

            ControlLabel {
                text: "ms"
                elide: Text.ElideNone
                enabled: autoRepeat.checked
            }

            ExpandingSpacer {}
        }
    }
}
