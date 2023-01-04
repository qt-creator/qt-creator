// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import QtQuick.Layouts 1.15
import HelperWidgets 2.0
import StudioTheme 1.0 as StudioTheme

Column {
    anchors.left: parent.left
    anchors.right: parent.right

    Section {
        anchors.left: parent.left
        anchors.right: parent.right
        caption: qsTr("Rectangle")

        SectionLayout {
            PropertyLabel { text: qsTr("Fill color") }

            ColorEditor {
                backendValue: backendValues.color
                supportGradient: backendValues.gradient.isAvailable
            }

            PropertyLabel {
                text: qsTr("Border color")
                visible: backendValues.border_color.isAvailable
            }

            ColorEditor {
                visible: backendValues.border_color.isAvailable
                backendValue: backendValues.border_color
                supportGradient: false
            }

            PropertyLabel {
                text: qsTr("Border width")
                blockedByTemplate: !backendValues.border_width.isAvailable
            }

            SecondColumnLayout {
                SpinBox {
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    width: implicitWidth
                    backendValue: backendValues.border_width
                    enabled: backendValue.isAvailable
                }

                ExpandingSpacer {}
            }

            PropertyLabel { text: qsTr("Radius") }

            SecondColumnLayout {
                SpinBox {
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    width: implicitWidth
                    backendValue: backendValues.radius
                    minimumValue: 0
                    maximumValue: Math.min(backendValues.height.value, backendValues.width.value) / 2
                }

                ExpandingSpacer {}
            }
        }
    }
}
