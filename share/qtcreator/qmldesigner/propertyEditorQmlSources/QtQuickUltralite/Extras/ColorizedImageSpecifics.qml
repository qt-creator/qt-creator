// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Layouts
import HelperWidgets
import StudioTheme as StudioTheme

//! [ColorizedImage compatibility]
Section {
    anchors.left: parent.left
    anchors.right: parent.right
    caption: qsTr("Colorized Image")

    SectionLayout {
        PropertyLabel { text: qsTr("Image color") }

        ColorEditor {
            backendValue: backendValues.color
            supportGradient: false
        }

        PropertyLabel { text: qsTr("Source") }

        SecondColumnLayout {
            UrlChooser {
                backendValue: backendValues.source
            }

            ExpandingSpacer {}
        }

        PropertyLabel { text: qsTr("Fill mode") }

        SecondColumnLayout {
            ComboBox {
                scope: "Image"
                model: ["Stretch", "PreserveAspectFit", "PreserveAspectCrop", "Tile", "TileVertically", "TileHorizontally", "Pad"]
                backendValue: backendValues.fillMode
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                width: implicitWidth
            }

            ExpandingSpacer {}
        }

        PropertyLabel { text: qsTr("Alignment H") }

        SecondColumnLayout {
            ComboBox {
                scope: "Image"
                model: ["AlignLeft", "AlignRight", "AlignHCenter"]
                backendValue: backendValues.horizontalAlignment
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                width: implicitWidth
            }

            ExpandingSpacer {}
        }

        PropertyLabel { text: qsTr("Alignment V") }

        SecondColumnLayout {
            ComboBox {
                scope: "Image"
                model: ["AlignTop", "AlignBottom", "AlignVCenter"]
                backendValue: backendValues.verticalAlignment
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                width: implicitWidth
            }

            ExpandingSpacer {}
        }
    }
}
//! [ColorizedImage compatibility]
