// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import QtQuick.Layouts 1.15
import HelperWidgets 2.0
import StudioTheme 1.0 as StudioTheme

Section {
    anchors.left: parent.left
    anchors.right: parent.right
    caption: qsTr("Connections")

    SectionLayout {
        PropertyLabel {
            text: qsTr("Enabled")
            tooltip: qsTr("Sets whether the component accepts change events.")
        }

        SecondColumnLayout {
            CheckBox {
                text: backendValues.enabled.valueToString
                backendValue: backendValues.enabled
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Ignore unknown signals")
            tooltip: qsTr("Ignores runtime errors produced by connections to non-existent signals.")
        }

        SecondColumnLayout {
            CheckBox {
                text: backendValues.ignoreUnknownSignals.valueToString
                backendValue: backendValues.ignoreUnknownSignals
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Target")
            tooltip: qsTr("Sets the component that sends the signal.")
        }

        SecondColumnLayout {
            ItemFilterComboBox {
                typeFilter: "QtQuick.Item"
                backendValue: backendValues.target
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}
        }
    }
}
