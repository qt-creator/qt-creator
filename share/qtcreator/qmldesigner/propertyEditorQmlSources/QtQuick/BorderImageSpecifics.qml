// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import QtQuick.Layouts 1.15
import HelperWidgets 2.0
import StudioControls 1.0 as StudioControls
import StudioTheme 1.0 as StudioTheme

Column {
    anchors.left: parent.left
    anchors.right: parent.right

    Section {
        caption: qsTr("Border Image")

        anchors.left: parent.left
        anchors.right: parent.right

        SectionLayout {
            PropertyLabel {
                text: qsTr("Source")
                tooltip: qsTr("Sets the source image for the border.")
            }

            SecondColumnLayout {
                UrlChooser {
                    backendValue: backendValues.source
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Source size")
                tooltip: qsTr("Sets the dimension of the border image.")
                blockedByTemplate: !backendValues.sourceSize.isAvailable
            }

            SecondColumnLayout {
                SpinBox {
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    backendValue: backendValues.sourceSize_width
                    minimumValue: -2000
                    maximumValue: 2000
                    decimals: 0
                    enabled: backendValue.isAvailable
                }

                Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

                ControlLabel {
                    //: The width of the object
                    text: qsTr("W", "width")
                    tooltip: qsTr("Width")
                    enabled: backendValues.sourceSize_width.isAvailable
                }

                Spacer { implicitWidth: StudioTheme.Values.controlGap }

                SpinBox {
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    backendValue: backendValues.sourceSize_height
                    minimumValue: -2000
                    maximumValue: 2000
                    decimals: 0
                    enabled: backendValue.isAvailable
                }

                Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

                ControlLabel {
                    //: The height of the object
                    text: qsTr("H", "height")
                    tooltip: qsTr("Height")
                    enabled: backendValues.sourceSize_height.isAvailable
                }
/*
                TODO QDS-4836
                Spacer { implicitWidth: StudioTheme.Values.controlGap }

                LinkIndicator2D {}
*/
                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Tile mode H")
                tooltip: qsTr("Sets the horizontal tiling mode.")
                blockedByTemplate: !backendValues.horizontalTileMode.isAvailable
            }

            SecondColumnLayout {
                ComboBox {
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    width: implicitWidth
                    backendValue: backendValues.horizontalTileMode
                    model: ["Stretch", "Repeat", "Round"]
                    scope: "BorderImage"
                    enabled: backendValue.isAvailable
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Tile mode V")
                tooltip: qsTr("Sets the vertical tiling mode.")
                blockedByTemplate: !backendValues.verticalTileMode.isAvailable
            }

            SecondColumnLayout {
                ComboBox {
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    width: implicitWidth
                    model: ["Stretch", "Repeat", "Round"]
                    backendValue: backendValues.verticalTileMode
                    scope: "BorderImage"
                    enabled: backendValue.isAvailable
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Border left")
                tooltip: qsTr("Sets the left border.")
            }

            SecondColumnLayout {
                SpinBox {
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    backendValue: backendValues.border_left
                    minimumValue: -2000
                    maximumValue: 2000
                    decimals: 0
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Border right")
                tooltip: qsTr("Sets the right border.")
            }

            SecondColumnLayout {
                SpinBox {
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    backendValue: backendValues.border_right
                    minimumValue: -2000
                    maximumValue: 2000
                    decimals: 0
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Border top")
                tooltip: qsTr("Sets the top border.")
            }

            SecondColumnLayout {
                SpinBox {
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    backendValue: backendValues.border_top
                    minimumValue: -2000
                    maximumValue: 2000
                    decimals: 0
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Border bottom")
                tooltip: qsTr("Sets the bottom border.")
            }

            SecondColumnLayout {
                SpinBox {
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    backendValue: backendValues.border_bottom
                    minimumValue: -2000
                    maximumValue: 2000
                    decimals: 0
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Mirror")
                tooltip: qsTr("Toggles if the image should be inverted horizontally.")
                blockedByTemplate: !backendValues.mirror.isAvailable
            }

            SecondColumnLayout {
                CheckBox {
                    text: backendValues.mirror.valueToString
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                  + StudioTheme.Values.actionIndicatorWidth
                    backendValue: backendValues.mirror
                    enabled: backendValue.isAvailable
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Cache")
                tooltip: qsTr("Toggles if the image is saved to the cache memory.")
                blockedByTemplate: !backendValues.cache.isAvailable
            }

            SecondColumnLayout {
                CheckBox {
                    text: backendValues.cache.valueToString
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    backendValue: backendValues.cache
                    enabled: backendValue.isAvailable
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Asynchronous")
                tooltip: qsTr("Toggles if the image is loaded after all the components in the design.")
                blockedByTemplate: !backendValues.asynchronous.isAvailable
            }

            SecondColumnLayout {
                CheckBox {
                    text: backendValues.asynchronous.valueToString
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    backendValue: backendValues.asynchronous
                    enabled: backendValue.isAvailable
                }

                ExpandingSpacer {}
            }
        }
    }
}
