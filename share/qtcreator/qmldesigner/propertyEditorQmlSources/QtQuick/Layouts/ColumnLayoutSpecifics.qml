// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import QtQuick.Layouts 1.15
import HelperWidgets 2.0
import StudioTheme 1.0 as StudioTheme

Section {
    anchors.left: parent.left
    anchors.right: parent.right
    caption: qsTr("Column Layout")

    SectionLayout {
        PropertyLabel { text: qsTr("Column spacing") }

        SecondColumnLayout {
            SpinBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.spacing
                minimumValue: -4000
                maximumValue: 4000
                decimals: 0
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Layout direction")
            blockedByTemplate: !backendValues.layoutDirection.isAvailable
        }

        SecondColumnLayout {
            ComboBox {
                scope: "Qt"
                model: ["LeftToRight", "RightToLeft"]
                backendValue: backendValues.layoutDirection
                enabled: backendValues.layoutDirection.isAvailable
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Uniform cell size")
            tooltip: qsTr("Toggles all cells to have a uniform size.")
            visible: majorQtQuickVersion === 6 && minorQtQuickVersion >= 6
        }

        SecondColumnLayout {
            CheckBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.uniformCellSizes
                visible: majorQtQuickVersion === 6 && minorQtQuickVersion >= 6
            }

            ExpandingSpacer {}
        }
    }
}
