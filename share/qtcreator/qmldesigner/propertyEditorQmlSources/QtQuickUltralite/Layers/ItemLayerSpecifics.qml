// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Layouts
import HelperWidgets
import StudioTheme as StudioTheme

//! [ItemLayer compatibility]
Section {
    anchors.left: parent.left
    anchors.right: parent.right
    caption: qsTr("Item Layer")

    SectionLayout {
        PropertyLabel { text: qsTr("Platform ID") }

        SecondColumnLayout {
            SpinBox {
                backendValue: backendValues.platformId
                minimumValue: 0
                maximumValue: 2000
                decimals: 0
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}
        }

        PropertyLabel { text: qsTr("Rendering hints") }

        SecondColumnLayout {
            ComboBox {
                model: ["OptimizeForSpeed", "OptimizeForSize", "StaticContents"]
                backendValue: backendValues.renderingHints
                scope: "ItemLayer"
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}
        }

        PropertyLabel { text: qsTr("Depth") }

        SecondColumnLayout {
            ComboBox {
                model: ["Bpp16", "Bpp16Alpha", "Bpp24", "Bpp32", "Bpp32Alpha"]
                backendValue: backendValues.depth
                scope: "ItemLayer"
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}
        }

        PropertyLabel { text: qsTr("Refresh interval") }

        SecondColumnLayout {
            SpinBox {
                backendValue: backendValues.refreshInterval
                minimumValue: 0
                maximumValue: 1000
                decimals: 0
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}
        }
    }
}
//! [ItemLayer compatibility]
