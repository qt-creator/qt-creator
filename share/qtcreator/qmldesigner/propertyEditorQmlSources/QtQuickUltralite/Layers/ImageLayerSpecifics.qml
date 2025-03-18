// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Layouts
import HelperWidgets
import StudioTheme as StudioTheme

//! [ImageLayer compatibility]
Section {
    anchors.left: parent.left
    anchors.right: parent.right
    caption: qsTr("Image Layer")

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
                scope: "ImageLayer"
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}
        }

        PropertyLabel { text: qsTr("Source") }

        SecondColumnLayout {
            UrlChooser {
                backendValue: backendValues.source
            }

            ExpandingSpacer {}
        }
    }
}
//! [ImageLayer compatibility]
