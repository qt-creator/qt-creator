// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Layouts
import HelperWidgets
import StudioTheme as StudioTheme

Section {
    anchors.left: parent.left
    anchors.right: parent.right
    caption: qsTr("String Mapper")

    SectionLayout {
        PropertyLabel { text: qsTr("Input") }

        SecondColumnLayout {
            SpinBox {
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.input
                decimals: 2
                minimumValue: Number.MIN_VALUE
                maximumValue: Number.MAX_VALUE
            }

            ExpandingSpacer {}
        }

        PropertyLabel { text: qsTr("Decimal places") }

        SecondColumnLayout {
            SpinBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.decimals
                minimumValue: 0
                maximumValue: 20
            }

            ExpandingSpacer {}
        }

        PropertyLabel { text: qsTr("Output text") }

        SecondColumnLayout {
            LineEdit {
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.text
                showTranslateCheckBox: false
            }

            ExpandingSpacer {}
        }
    }
}
