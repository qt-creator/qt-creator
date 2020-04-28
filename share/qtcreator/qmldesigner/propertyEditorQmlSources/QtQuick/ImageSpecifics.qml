/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

import QtQuick 2.1
import HelperWidgets 2.0
import QtQuick.Layouts 1.0

Column {
    anchors.left: parent.left
    anchors.right: parent.right

    Section {
        anchors.left: parent.left
        anchors.right: parent.right
        caption: qsTr("Image")

        SectionLayout {
            Label {
                text: qsTr("Source")
            }

            SecondColumnLayout {
                UrlChooser {
                    Layout.fillWidth: true
                    backendValue: backendValues.source
                }

                ExpandingSpacer {
                }
            }

            Label {
                text: qsTr("Fill mode")
            }

            SecondColumnLayout {
                ComboBox {
                    scope: "Image"
                    model: ["Stretch", "PreserveAspectFit", "PreserveAspectCrop", "Tile", "TileVertically", "TileHorizontally", "Pad"]
                    backendValue: backendValues.fillMode
                    implicitWidth: 180
                    Layout.fillWidth: true
                }

                ExpandingSpacer {
                }
            }

            Label {
                text: qsTr("Source size")
            }

            SecondColumnLayout {
                Label {
                    text: "W"
                    width: 12
                }

                SpinBox {
                    backendValue: backendValues.sourceSize_width
                    minimumValue: 0
                    maximumValue: 8192
                    decimals: 0
                }

                Item {
                    width: 4
                    height: 4
                }

                Label {
                    text: "H"
                    width: 12
                }

                SpinBox {
                    backendValue: backendValues.sourceSize_height
                    minimumValue: 0
                    maximumValue: 8192
                    decimals: 0
                }

                ExpandingSpacer {
                }
            }

            Label {
                text: qsTr("Horizontal alignment")
            }

            SecondColumnLayout {
                ComboBox {
                    scope: "Image"
                    model: ["AlignLeft", "AlignRight", "AlignHCenter"]
                    backendValue: backendValues.horizontalAlignment
                    implicitWidth: 180
                    Layout.fillWidth: true
                }

                ExpandingSpacer {
                }
            }

            Label {
                text: qsTr("Vertical alignment")
            }

            SecondColumnLayout {
                ComboBox {
                    scope: "Image"
                    model: ["AlignTop", "AlignBottom", "AlignVCenter"]
                    backendValue: backendValues.verticalAlignment
                    implicitWidth: 180
                    Layout.fillWidth: true
                }

                ExpandingSpacer {
                }
            }

            Label {
                text: qsTr("Asynchronous")
                tooltip: qsTr("Specifies that images on the local filesystem should be loaded asynchronously in a separate thread.")
                disabledState: !backendValues.asynchronous.isAvailable
            }

            SecondColumnLayout {
                CheckBox {
                    enabled: backendValues.asynchronous.isAvailable
                    text: backendValues.asynchronous.valueToString
                    backendValue: backendValues.asynchronous
                    implicitWidth: 180
                }
                ExpandingSpacer {}
            }

            Label {
                text: qsTr("Auto transform")
                tooltip: qsTr("Specifies whether the image should automatically apply image transformation metadata such as EXIF orientation.")
                disabledState: !backendValues.autoTransform.isAvailable
            }

            SecondColumnLayout {
                CheckBox {
                    enabled: backendValues.autoTransform.isAvailable
                    text: backendValues.autoTransform.valueToString
                    backendValue: backendValues.autoTransform
                    implicitWidth: 180
                }
                ExpandingSpacer {}
            }

            Label {
                text: qsTr("Cache")
                tooltip: qsTr("Specifies whether the image should be cached.")
                disabledState: !backendValues.cache.isAvailable
            }

            SecondColumnLayout {
                CheckBox {
                    enabled: backendValues.cache.isAvailable
                    text: backendValues.cache.valueToString
                    backendValue: backendValues.cache
                    implicitWidth: 180
                }
                ExpandingSpacer {}
            }

            Label {
                text: qsTr("Mipmap")
                tooltip: qsTr("Specifies whether the image uses mipmap filtering when scaled or transformed.")
                disabledState: !backendValues.mipmap.isAvailable
            }

            SecondColumnLayout {
                CheckBox {
                    enabled: backendValues.mipmap.isAvailable
                    text: backendValues.mipmap.valueToString
                    backendValue: backendValues.mipmap
                    implicitWidth: 180
                }
                ExpandingSpacer {}
            }

            Label {
                text: qsTr("Mirror")
                tooltip: qsTr("Specifies whether the image should be horizontally inverted.")
                disabledState: !backendValues.mirror.isAvailable
            }

            SecondColumnLayout {
                CheckBox {
                    enabled: backendValues.mirror.isAvailable
                    text: backendValues.mirror.valueToString
                    backendValue: backendValues.mirror
                    implicitWidth: 180
                }
                ExpandingSpacer {}
            }

            Label {
                text: qsTr("Smooth")
                tooltip: qsTr("Specifies whether the image is smoothly filtered when scaled or transformed.")
                disabledState: !backendValues.smooth.isAvailable
            }

            SecondColumnLayout {
                CheckBox {
                    enabled: backendValues.smooth.isAvailable
                    text: backendValues.smooth.valueToString
                    backendValue: backendValues.smooth
                    implicitWidth: 180
                }
                ExpandingSpacer {}
            }
        }
    }
}
