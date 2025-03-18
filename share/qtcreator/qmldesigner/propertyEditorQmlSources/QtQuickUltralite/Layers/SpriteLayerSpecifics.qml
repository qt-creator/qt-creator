// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Layouts
import HelperWidgets
import StudioTheme as StudioTheme

//! [SpriteLayer compatibility]
Section {
    anchors.left: parent.left
    anchors.right: parent.right
    caption: qsTr("Sprite Layer")

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

        PropertyLabel { text: qsTr("Depth") }

        SecondColumnLayout {
            ComboBox {
                model: ["Bpp8", "Bpp16", "Bpp16Alpha", "Bpp24", "Bpp32", "Bpp32Alpha"]
                backendValue: backendValues.depth
                scope: "SpriteLayer"
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                width: implicitWidth
            }

            ExpandingSpacer {}
        }
    }
}
//! [SpriteLayer compatibility]
