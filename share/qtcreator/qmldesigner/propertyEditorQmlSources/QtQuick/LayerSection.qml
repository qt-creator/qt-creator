// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
            tooltip: qsTr("Toggles if the component is layered.")
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
            tooltip: qsTr("Sets the name of the effect's source texture property.")
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
            tooltip: qsTr("Sets the number of multisample renderings in the layer.")
        }

        SecondColumnLayout {
            ComboBox {
                id: samplesComboBox
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                width: implicitWidth
                model: [0, 2, 4, 8, 16]
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
            tooltip: qsTr("Sets which effect is applied.")
        }

        SecondColumnLayout {
            ItemFilterComboBox {
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                width: implicitWidth
                typeFilter: "QtQuick.Item"
                backendValue: backendValues.layer_effect
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Format")
            tooltip: qsTr("Sets the internal OpenGL format for the texture.")
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
            tooltip: qsTr("Sets the requested pixel size of the layer's texture.")
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
                tooltip: qsTr("Width.")
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
                tooltip: qsTr("Height.")
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
            tooltip: qsTr("Sets the OpenGL wrap modes associated with the texture.")
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
            tooltip: qsTr("Toggles if mipmaps are generated for the texture.")
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
            tooltip: qsTr("Toggles if the layer transforms smoothly.")
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

        PropertyLabel {
            text: qsTr("Source rectangle")
            tooltip: qsTr("Sets the rectangular area of the component that should\n"
                        + "be rendered into the texture.")
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
    }
}
