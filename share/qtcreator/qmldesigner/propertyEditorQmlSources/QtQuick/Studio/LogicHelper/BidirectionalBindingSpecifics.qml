// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Layouts
import HelperWidgets
import StudioTheme as StudioTheme

Section {
    anchors.left: parent.left
    anchors.right: parent.right
    caption: qsTr("Bidrectional Binding")

    SectionLayout {
        PropertyLabel {
            text: qsTr("Target 01")
            tooltip: qsTr("Target of the binding.")
        }

        SecondColumnLayout {
            ItemFilterComboBox {
                typeFilter: "QtQml.QtObject"
                validator: RegExpValidator { regExp: /(^$|^[a-z_]\w*)/ }
                backendValue: backendValues.target01
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}
        }

        PropertyLabel { text: qsTr("Property 01") }

        SecondColumnLayout {
            LineEdit {
                backendValue: backendValues.property01
                showTranslateCheckBox: false
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Target 02")
            tooltip: qsTr("Target Flow Item whose state is changed.")
        }

        SecondColumnLayout {
            ItemFilterComboBox {
                typeFilter: "QtQml.QtObject"
                validator: RegExpValidator { regExp: /(^$|^[a-z_]\w*)/ }
                backendValue: backendValues.target02
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}
        }

        PropertyLabel { text: qsTr("Property 02") }

        SecondColumnLayout {
            LineEdit {
                backendValue: backendValues.property02
                showTranslateCheckBox: false
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}
        }
    }
}
