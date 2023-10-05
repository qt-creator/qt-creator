// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import QtQuick.Layouts 1.15
import HelperWidgets 2.0
import StudioTheme 1.0 as StudioTheme

Section {
    caption: qsTr("Image")

    anchors.left: parent.left
    anchors.right: parent.right

    SectionLayout {
        PropertyLabel {
            text: qsTr("Source")
            tooltip: qsTr("Adds an image from the local file system.")
        }

        SecondColumnLayout {
            UrlChooser {
                backendValue: backendValues.source
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Fill mode")
            tooltip: qsTr("Sets how the image fits in the content box.")
        }

        SecondColumnLayout {
            ComboBox {
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                width: implicitWidth
                scope: "Image"
                model: ["Stretch", "PreserveAspectFit", "PreserveAspectCrop", "Tile", "TileVertically", "TileHorizontally", "Pad"]
                backendValue: backendValues.fillMode
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Source size")
            tooltip: qsTr("Sets the width and height of the image.")
            blockedByTemplate: !backendValues.sourceSize.isAvailable
        }

        SecondColumnLayout {
            SpinBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.sourceSize_width
                minimumValue: 0
                maximumValue: 8192
                decimals: 0
                enabled: backendValue.isAvailable
            }

            Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

            ControlLabel {
                //: The width of the object
                text: qsTr("W", "width")
                tooltip: qsTr("Width.")
                enabled: backendValues.sourceSize_width.isAvailable
            }

            Spacer { implicitWidth: StudioTheme.Values.controlGap }

            SpinBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.sourceSize_height
                minimumValue: 0
                maximumValue: 8192
                decimals: 0
                enabled: backendValue.isAvailable
            }

            Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

            ControlLabel {
                //: The height of the object
                text: qsTr("H", "height")
                tooltip: qsTr("Height.")
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
            text: qsTr("Alignment H")
            tooltip: qsTr("Sets the horizontal alignment of the image.")
        }

        SecondColumnLayout {
            ComboBox {
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                width: implicitWidth
                scope: "Image"
                model: ["AlignLeft", "AlignRight", "AlignHCenter"]
                backendValue: backendValues.horizontalAlignment
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Alignment V")
            tooltip: qsTr("Sets the vertical alignment of the image.")
        }

        SecondColumnLayout {
            ComboBox {
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                width: implicitWidth
                scope: "Image"
                model: ["AlignTop", "AlignBottom", "AlignVCenter"]
                backendValue: backendValues.verticalAlignment
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Asynchronous")
            tooltip: qsTr("Loads images on the local filesystem asynchronously in a separate thread.")
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

        PropertyLabel {
            text: qsTr("Auto transform")
            tooltip: qsTr("Automatically applies image transformation metadata such as EXIF orientation.")
            blockedByTemplate: !backendValues.autoTransform.isAvailable
        }

        SecondColumnLayout {
            CheckBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                text: backendValues.autoTransform.valueToString
                backendValue: backendValues.autoTransform
                enabled: backendValue.isAvailable
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Cache")
            tooltip: qsTr("Caches the image.")
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
            text: qsTr("Mipmap")
            tooltip: qsTr("Uses mipmap filtering when the image is scaled or transformed.")
            blockedByTemplate: !backendValues.mipmap.isAvailable
        }

        SecondColumnLayout {
            CheckBox {
                text: backendValues.mipmap.valueToString
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.mipmap
                enabled: backendValue.isAvailable
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Mirror")
            tooltip: qsTr("Inverts the image horizontally.")
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
    }
}
