/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

import QtQuick 2.15
import QtQuick.Layouts 1.15
import HelperWidgets 2.0
import StudioTheme 1.0 as StudioTheme

Section {
    caption: qsTr("Image")

    anchors.left: parent.left
    anchors.right: parent.right

    SectionLayout {
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
                enabled: backendValues.sourceSize_height.isAvailable
            }
/*
            TODO QDS-4836
            Spacer { implicitWidth: StudioTheme.Values.controlGap }

            LinkIndicator2D {}
*/
            ExpandingSpacer {}
        }

        PropertyLabel { text: qsTr("Alignment H") }

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

        PropertyLabel { text: qsTr("Alignment V") }

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

        PropertyLabel {
            text: qsTr("Smooth")
            tooltip: qsTr("Smoothly filters the image when it is scaled or transformed.")
            blockedByTemplate: !backendValues.smooth.isAvailable
        }

        SecondColumnLayout {
            CheckBox {
                text: backendValues.smooth.valueToString
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.smooth
                enabled: backendValue.isAvailable
            }

            ExpandingSpacer {}
        }
    }
}
