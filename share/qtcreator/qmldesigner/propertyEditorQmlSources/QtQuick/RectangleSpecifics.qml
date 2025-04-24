// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Layouts
import HelperWidgets
import StudioTheme as StudioTheme

Column {
    anchors.left: parent.left
    anchors.right: parent.right

    Section {
        anchors.left: parent.left
        anchors.right: parent.right
        caption: qsTr("Rectangle")

        SectionLayout {
            PropertyLabel {
                text: qsTr("Fill color")
                tooltip: qsTr("Sets the color for the background.")
            }

            ColorEditor {
                backendValue: backendValues.color
                supportGradient: backendValues.gradient.isAvailable
            }

            PropertyLabel {
                text: qsTr("Border color")
                tooltip: qsTr("Sets the color for the border.")
                visible: backendValues.border_color.isAvailable
            }

            ColorEditor {
                visible: backendValues.border_color.isAvailable
                backendValue: backendValues.border_color
                supportGradient: false
            }

            PropertyLabel {
                text: qsTr("Border width")
                tooltip: qsTr("Sets the border width.")
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

            PropertyLabel {
                text: qsTr("Radius")
                tooltip: qsTr("Sets the radius by which the corners get rounded.")
            }

            SecondColumnLayout {
                SpinBox {
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    width: implicitWidth
                    backendValue: backendValues.radius
                    minimumValue: 0
                    maximumValue: 0xffff
                }

                ExpandingSpacer {}
            }
        }
    }

    CornerRadiusSection {
        id: cornerRadiusSection
        property bool radiiAvailable: backendValues.topLeftRadius.isAvailable
                                // && backendValues.topRightRadius.isAvailable
                                // && backendValues.bottomLeftRadius.isAvailable
                                // && backendValues.bottomRightRadius.isAvailable

        visible: majorQtQuickVersion >= 6 && minorQtQuickVersion >= 7 && cornerRadiusSection.radiiAvailable
    }
}
