// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0 WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import QtQuick.Layouts 1.15
import HelperWidgets 2.0
import StudioTheme 1.0 as StudioTheme

Section {
    id: root
    caption: qsTr("Audio Room")
    width: parent.width

    property var materialModel: [
        "Transparent", "AcousticCeilingTiles", "BrickBare", "BrickPainted",
        "ConcreteBlockCoarse", "ConcreteBlockPainted", "CurtainHeavy",
        "FiberGlassInsulation", "GlassThin", "GlassThick", "Grass",
        "LinoleumOnConcrete", "Marble", "Metal", "ParquetOnConcrete",
        "PlasterRough", "PlasterSmooth", "PlywoodPanel", "PolishedConcreteOrTile",
        "Sheetrock", "WaterOrIceSurface", "WoodCeiling", "WoodPanel", "Uniform"
    ]

    SectionLayout {
        PropertyLabel {
            text: qsTr("Dimensions")
            tooltip: qsTr("Sets the dimensions of the room in 3D space.")
        }

        SecondColumnLayout {
            SpinBox {
                minimumValue: -9999999
                maximumValue: 9999999
                decimals: 2
                backendValue: backendValues.dimensions_x
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

            ControlLabel {
                text: "X"
                color: StudioTheme.Values.theme3DAxisXColor
            }

            ExpandingSpacer {}
        }

        PropertyLabel {}

        SecondColumnLayout {
            SpinBox {
                minimumValue: -9999999
                maximumValue: 9999999
                decimals: 2
                backendValue: backendValues.dimensions_y
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

            ControlLabel {
                text: "Y"
                color: StudioTheme.Values.theme3DAxisYColor
            }

            ExpandingSpacer {}
        }

        PropertyLabel {}

        SecondColumnLayout {
            SpinBox {
                minimumValue: -9999999
                maximumValue: 9999999
                decimals: 2
                backendValue: backendValues.dimensions_z
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

            ControlLabel {
                text: "Z"
                color: StudioTheme.Values.theme3DAxisZColor
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Reflection Gain")
            tooltip: qsTr("Sets the gain factor for reflections generated in this room.\nA value from 0 to 1 will dampen reflections, while a value larger than 1 will apply a gain to reflections, making them louder.")
        }

        SecondColumnLayout {
            SpinBox {
                minimumValue: 0
                maximumValue: 999999
                decimals: 2
                stepSize: 0.1
                backendValue: backendValues.reflectionGain
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Reverb Gain")
            tooltip: qsTr("Sets the gain factor for reverb generated in this room.\nA value from 0 to 1 will dampen reverb, while a value larger than 1 will apply a gain to the reverb, making it louder.")
        }

        SecondColumnLayout {
            SpinBox {
                minimumValue: 0
                maximumValue: 999999
                decimals: 2
                stepSize: 0.1
                backendValue: backendValues.reverbGain
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Reverb Time")
            tooltip: qsTr("Sets the factor to be applies to all reverb timings generated for this room.\nLarger values will lead to longer reverb timings, making the room sound larger.")
        }

        SecondColumnLayout {
            SpinBox {
                minimumValue: 0
                maximumValue: 999999
                decimals: 2
                stepSize: 0.1
                backendValue: backendValues.reverbTime
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Reverb Brightness")
            tooltip: qsTr("Sets the brightness factor to be applied to the generated reverb.\nA positive value will increase reverb for higher frequencies and dampen lower frequencies, a negative value does the reverse.")
        }

        SecondColumnLayout {
            SpinBox {
                minimumValue: -999999
                maximumValue: 999999
                decimals: 2
                stepSize: 0.1
                backendValue: backendValues.reverbBrightness
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Left Material")
            tooltip: qsTr("Sets the material to use for the left (negative x) side of the room.")
        }

        SecondColumnLayout {
            ComboBox {
                scope: "AudioRoom"
                model: root.materialModel
                backendValue: backendValues.leftMaterial
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}

        }

        PropertyLabel {
            text: qsTr("Right Material")
            tooltip: qsTr("Sets the material to use for the right (positive x) side of the room.")
        }

        SecondColumnLayout {
            ComboBox {
                scope: "AudioRoom"
                model: root.materialModel
                backendValue: backendValues.rightMaterial
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}

        }

        PropertyLabel {
            text: qsTr("Floor Material")
            tooltip: qsTr("Sets the material to use for the floor (negative y) side of the room.")
        }

        SecondColumnLayout {
            ComboBox {
                scope: "AudioRoom"
                model: root.materialModel
                backendValue: backendValues.floorMaterial
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}

        }

        PropertyLabel {
            text: qsTr("Ceiling Material")
            tooltip: qsTr("Sets the material to use for the ceiling (positive y) side of the room.")
        }

        SecondColumnLayout {
            ComboBox {
                scope: "AudioRoom"
                model: root.materialModel
                backendValue: backendValues.ceilingMaterial
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}

        }

        PropertyLabel {
            text: qsTr("Back Material")
            tooltip: qsTr("Sets the material to use for the back (negative z) side of the room.")
        }

        SecondColumnLayout {
            ComboBox {
                scope: "AudioRoom"
                model: root.materialModel
                backendValue: backendValues.backMaterial
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}

        }

        PropertyLabel {
            text: qsTr("Front Material")
            tooltip: qsTr("Sets the material to use for the front (positive z) side of the room.")
        }

        SecondColumnLayout {
            ComboBox {
                scope: "AudioRoom"
                model: root.materialModel
                backendValue: backendValues.frontMaterial
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}

        }
    }
}
