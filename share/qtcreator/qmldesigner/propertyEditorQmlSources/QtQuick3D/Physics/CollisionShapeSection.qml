// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Layouts
import HelperWidgets
import StudioTheme as StudioTheme

Column {
    width: parent.width

    Section {
        width: parent.width
        caption: qsTr("Collision Shape")

        SectionLayout {
            PropertyLabel {
                text: qsTr("Debug Draw")
                tooltip: qsTr("Draws the collision shape in the scene view.")
            }

            SecondColumnLayout {
                CheckBox {
                    text: backendValues.enableDebugDraw.valueToString
                    backendValue: backendValues.enableDebugDraw
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                }

                ExpandingSpacer {}
            }
        }
    }
}
