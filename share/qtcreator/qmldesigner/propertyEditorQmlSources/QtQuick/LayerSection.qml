/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

import QtQuick 2.0
import HelperWidgets 2.0
import QtQuick.Layouts 1.0

Section {
    anchors.left: parent.left
    anchors.right: parent.right
    caption: qsTr("Layer")
    visible: backendValues.layer_effect.isAvailable

    SectionLayout {
        columns: 2

        Label {
            text: qsTr("Effect")
            tooltip: qsTr("Sets the effect that is applied to this layer.")
        }
        SecondColumnLayout {
            ItemFilterComboBox {
                typeFilter: "QtQuick.Item"
                validator: RegExpValidator { regExp: /(^$|^[a-z_]\w*)/ }
                backendValue: backendValues.layer_effect
                Layout.fillWidth: true
            }

            ExpandingSpacer {
            }
        }

        Label {
            text: qsTr("Enabled")
            tooltip: qsTr("Sets whether the item is layered or not.")
        }
        SecondColumnLayout {
            CheckBox {
                text: backendValues.layer_enabled.valueToString
                backendValue: backendValues.layer_enabled
                Layout.fillWidth: true
            }

            ExpandingSpacer {
            }
        }

        Label {
            text: qsTr("Format")
            tooltip: qsTr("Defines the internal OpenGL format of the texture.")
        }
        SecondColumnLayout {
            ComboBox {
                scope: "ShaderEffectSource"
                model: ["Alpha", "RGB", "RGBA"]
                backendValue: backendValues.layer_format
                Layout.fillWidth: true
            }

            ExpandingSpacer {
            }
        }

        Label {
            text: qsTr("Mipmap")
            tooltip: qsTr("Enables the generation of mipmaps for the texture.")
        }
        SecondColumnLayout {
            CheckBox {
                text: backendValues.layer_mipmap.valueToString
                backendValue: backendValues.layer_mipmap
                Layout.fillWidth: true
            }

            ExpandingSpacer {
            }
        }

        Label {
            text: qsTr("Sampler name")
            tooltip: qsTr("Sets the name of the effect's source texture property.")
        }
        SecondColumnLayout {
            LineEdit {
                backendValue: backendValues.layer_samplerName
                text: backendValues.layer_samplerName.valueToString
                Layout.fillWidth: true
                showTranslateCheckBox: false
            }

            ExpandingSpacer {
            }
        }

        Label {
            text: qsTr("Samples")
            tooltip: qsTr("Allows requesting multisampled rendering in the layer.")
        }
        SecondColumnLayout {
            ComboBox {
                id: samplesComboBox
                model: [2, 4, 8, 16]
                backendValue: backendValues.layer_samples
                manualMapping: true
                Layout.fillWidth: true

                onValueFromBackendChanged: {
                    if (!samplesComboBox.__isCompleted)
                        return

                    samplesComboBox.syncIndexToBackendValue()
                }
                onCompressedActivated: {
                    if (!samplesComboBox.__isCompleted)
                        return

                    if (samplesComboBox.block)
                        return

                    backendValues.layer_samples.value = samplesComboBox.model[samplesComboBox.currentIndex]
                }
                Component.onCompleted: samplesComboBox.syncIndexToBackendValue()

                function syncIndexToBackendValue() {
                    samplesComboBox.block = true
                    samplesComboBox.currentIndex = samplesComboBox.model.indexOf(backendValues.layer_samples.value)
                    samplesComboBox.block = false
                }
            }

            ExpandingSpacer {
            }
        }

        Label {
            text: qsTr("Smooth")
            tooltip: qsTr("Sets whether the layer is smoothly transformed.")
        }
        SecondColumnLayout {
            CheckBox {
                text: backendValues.layer_smooth.valueToString
                backendValue: backendValues.layer_smooth
                Layout.fillWidth: true
            }

            ExpandingSpacer {
            }
        }

/*
        Label {
            text: qsTr("Source rectangle")
            tooltip: qsTr("TODO.")
        }
        SecondColumnLayout {
            Label {
                text: "X"
                width: 12
            }
            SpinBox {
                backendValue: backendValues.layer_sourceRect_x
                maximumValue: 0xffff
                minimumValue: -0xffff
                decimals: 0
                realDragRange: 5000
            }

            Item {
                width: 4
                height: 4
            }

            Label {
                text: "Y"
                width: 12
            }
            SpinBox {
                backendValue: backendValues.layer_sourceRect_y
                maximumValue: 0xffff
                minimumValue: -0xffff
                decimals: 0
                realDragRange: 5000
            }

            ExpandingSpacer {
            }
        }

        Item {
            width: 4
            height: 4
        }
        SecondColumnLayout {
            Layout.fillWidth: true

            Label {
                text: "W"
                width: 12
            }
            SpinBox {
                backendValue: backendValues.layer_sourceRect_width
                maximumValue: 0xffff
                minimumValue: 0
                decimals: 0
                realDragRange: 5000
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
                backendValue: backendValues.layer_sourceRect_height
                maximumValue: 0xffff
                minimumValue: 0
                decimals: 0
                realDragRange: 5000
            }

            ExpandingSpacer {
            }
        }
*/

        Label {
            text: qsTr("Texture mirroring")
            tooltip: qsTr("Defines how the generated OpenGL texture should be mirrored.")
        }
        SecondColumnLayout {
            ComboBox {
                scope: "ShaderEffectSource"
                model: ["NoMirroring", "MirrorHorizontally", "MirrorVertically"]
                backendValue: backendValues.layer_textureMirroring
                Layout.fillWidth: true
            }

            ExpandingSpacer {
            }
        }

        Label {
            text: qsTr("Texture size")
            tooltip: qsTr("Sets the requested pixel size of the layers texture.")
        }
        SecondColumnLayout {
            Label {
                text: "W"
                width: 12
            }
            SpinBox {
                backendValue: backendValues.layer_textureSize_width
                minimumValue: 0
                maximumValue: 2000
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
                backendValue: backendValues.layer_textureSize_height
                minimumValue: 0
                maximumValue: 2000
                decimals: 0
            }

            ExpandingSpacer {
            }
        }

        Label {
            text: qsTr("Wrap mode")
            tooltip: qsTr("Defines the OpenGL wrap modes associated with the texture.")
        }
        SecondColumnLayout {
            ComboBox {
                scope: "ShaderEffectSource"
                model: ["ClampToEdge", "RepeatHorizontally", "RepeatVertically", "Repeat"]
                backendValue: backendValues.layer_wrapMode
                Layout.fillWidth: true
            }

            ExpandingSpacer {
            }
        }
    }
}
