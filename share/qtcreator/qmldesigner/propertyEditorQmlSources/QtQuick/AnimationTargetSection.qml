// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import HelperWidgets 2.0
import QtQuick.Layouts 1.15
import StudioControls 1.0 as StudioControls
import StudioTheme 1.0 as StudioTheme

Section {
    id: section
    caption: qsTr("Animation Targets")

    anchors.left: parent.left
    anchors.right: parent.right

    SectionLayout {
        PropertyLabel {
            text: qsTr("Target")
            tooltip: qsTr("Target to animate the properties of.")
        }

        SecondColumnLayout {
            ItemFilterComboBox {
                typeFilter: "QtQuick.QtObject"
                backendValue: backendValues.target
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Property")
            tooltip: qsTr("Property to animate.")
        }

        SecondColumnLayout {
            LineEdit {
                backendValue: backendValues.property
                showTranslateCheckBox: false
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Properties")
            tooltip: qsTr("Properties to animate.")
        }

        SecondColumnLayout {
            LineEdit {
                backendValue: backendValues.properties
                showTranslateCheckBox: false
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}
        }
    }
}
