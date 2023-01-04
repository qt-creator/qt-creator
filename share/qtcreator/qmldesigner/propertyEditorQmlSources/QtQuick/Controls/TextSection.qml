// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import HelperWidgets 2.0
import QtQuick.Layouts 1.15
import StudioTheme 1.0 as StudioTheme

Section {
    width: parent.width
    caption: qsTr("Text Area")

    SectionLayout {
        PropertyLabel {
            text: qsTr("Placeholder text")
            tooltip: qsTr("Placeholder text displayed when the editor is empty.")
        }

        SecondColumnLayout {
            LineEdit {
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                + StudioTheme.Values.actionIndicatorWidth
                width: implicitWidth
                backendValue: backendValues.placeholderText
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Placeholder color")
            tooltip: qsTr("Placeholder text color.")
        }

        ColorEditor {
            backendValue: backendValues.placeholderTextColor
            supportGradient: false
        }

        PropertyLabel {
            text: qsTr("Hover")
            tooltip: qsTr("Whether text area accepts hover events.")
        }

        SecondColumnLayout {
            CheckBox {
                text: backendValues.hoverEnabled.valueToString
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.hoverEnabled
            }

            ExpandingSpacer {}
        }
    }
}
