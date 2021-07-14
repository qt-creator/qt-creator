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
    anchors.left: parent.left
    anchors.right: parent.right
    caption: qsTr("Layer")
    visible: backendValues.layer_effect.isAvailable

    SectionLayout {
        PropertyLabel {
            text: qsTr("Enabled")
            tooltip: qsTr("Whether the component is layered or not.")
        }

        SecondColumnLayout {
            CheckBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                text: backendValues.layer_enabled.valueToString
                backendValue: backendValues.layer_enabled
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Sampler name")
            tooltip: qsTr("Name of the effect's source texture property.")
        }

        SecondColumnLayout {
            LineEdit {
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                width: implicitWidth
                backendValue: backendValues.layer_samplerName
                text: backendValues.layer_samplerName.valueToString
                showTranslateCheckBox: false
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Samples")
            tooltip: qsTr("Allows requesting multisampled rendering in the layer.")
        }

        SecondColumnLayout {
            ComboBox {
                id: samplesComboBox
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                width: implicitWidth
                model: [2, 4, 8, 16]
                backendValue: backendValues.layer_samples
                manualMapping: true

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

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Effect")
            tooltip: qsTr("Applies the effect to this layer.")
        }

        SecondColumnLayout {
            ItemFilterComboBox {
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                width: implicitWidth
                typeFilter: "QtQuick.Item"
                validator: RegExpValidator { regExp: /(^$|^[a-z_]\w*)/ }
                backendValue: backendValues.layer_effect
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Format")
            tooltip: qsTr("Internal OpenGL format of the texture.")
        }

        SecondColumnLayout {
            ComboBox {
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                width: implicitWidth
                scope: "ShaderEffectSource"
                model: ["Alpha", "RGB", "RGBA"]
                backendValue: backendValues.layer_format
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Texture size")
            tooltip: qsTr("Requested pixel size of the layer's texture.")
        }

        SecondColumnLayout {
            SpinBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.layer_textureSize_width
                minimumValue: 0
                maximumValue: 2000
                decimals: 0
            }

            Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

            ControlLabel {
                //: The width of the object
                text: qsTr("W", "width")
            }

            Spacer { implicitWidth: StudioTheme.Values.controlGap }

            SpinBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.layer_textureSize_height
                minimumValue: 0
                maximumValue: 2000
                decimals: 0
            }

            Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

            ControlLabel {
                //: The height of the object
                text: qsTr("H", "height")
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Texture mirroring")
            tooltip: qsTr("Sets how the generated OpenGL texture should be mirrored.")
        }

        SecondColumnLayout {
            ComboBox {
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                width: implicitWidth
                scope: "ShaderEffectSource"
                model: ["NoMirroring", "MirrorHorizontally", "MirrorVertically"]
                backendValue: backendValues.layer_textureMirroring
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Wrap mode")
            tooltip: qsTr("OpenGL wrap modes associated with the texture.")
        }

        SecondColumnLayout {
            ComboBox {
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                width: implicitWidth
                scope: "ShaderEffectSource"
                model: ["ClampToEdge", "RepeatHorizontally", "RepeatVertically", "Repeat"]
                backendValue: backendValues.layer_wrapMode
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Mipmap")
            tooltip: qsTr("Generates mipmaps for the texture.")
        }

        SecondColumnLayout {
            CheckBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                text: backendValues.layer_mipmap.valueToString
                backendValue: backendValues.layer_mipmap
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Smooth")
            tooltip: qsTr("Transforms the layer smoothly.")
        }

        SecondColumnLayout {
            CheckBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                text: backendValues.layer_smooth.valueToString
                backendValue: backendValues.layer_smooth
            }

            ExpandingSpacer {}
        }
/*
        PropertyLabel {
            text: qsTr("Source Rectangle")
            tooltip: qsTr("TODO.")
        }

        SecondColumnLayout {
            SpinBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.layer_sourceRect_x
                maximumValue: 0xffff
                minimumValue: -0xffff
                decimals: 0
            }

            Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

            ControlLabel { text: "X" }

            Spacer { implicitWidth: StudioTheme.Values.controlGap }

            SpinBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.layer_sourceRect_y
                maximumValue: 0xffff
                minimumValue: -0xffff
                decimals: 0
            }

            Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

            ControlLabel { text: "Y" }

            ExpandingSpacer {}
        }

        PropertyLabel {}

        SecondColumnLayout {
            SpinBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.layer_sourceRect_width
                maximumValue: 0xffff
                minimumValue: 0
                decimals: 0
            }

            Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

            ControlLabel {
                //: The width of the object
                text: qsTr("W", "width")
            }

            Spacer { implicitWidth: StudioTheme.Values.controlGap }

            SpinBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.layer_sourceRect_height
                maximumValue: 0xffff
                minimumValue: 0
                decimals: 0
                realDragRange: 5000
            }

            Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

            ControlLabel {
                //: The height of the object
                text: qsTr("H", "height")
            }

            ExpandingSpacer {}
        }
*/
    }
}
