// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0 WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import QtQuick.Layouts 1.15
import HelperWidgets 2.0
import StudioTheme 1.0 as StudioTheme

Section {
    caption: qsTr("Spatial Sound")
    width: parent.width

    SectionLayout {
        PropertyLabel {
            text: qsTr("Source")
            tooltip: qsTr("The source file for the sound to be played.")
        }

        SecondColumnLayout {
            UrlChooser {
                id: sourceUrlChooser
                backendValue: backendValues.source
                filter: "*.wav *.mp3"
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Volume")
            tooltip: qsTr("Set the overall volume for this sound source.\nValues between 0 and 1 will attenuate the sound, while values above 1 provide an additional gain boost.")
        }

        SecondColumnLayout {
            SpinBox {
                minimumValue: 0
                maximumValue: 999999
                decimals: 2
                stepSize: 0.1
                backendValue: backendValues.volume
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Loops")
            tooltip: qsTr("Sets how often the sound is played before the player stops.\nBind to SpatialSound.Infinite to loop the current sound forever.")
        }

        SecondColumnLayout {
            SpinBox {
                minimumValue: 1
                maximumValue: 999999
                decimals: 0
                stepSize: 1
                backendValue: backendValues.loops
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Auto Play")
            tooltip: qsTr("Sets whether the sound should automatically start playing when a source gets specified.")
        }

        SecondColumnLayout {
            CheckBox {
                text: backendValues.autoPlay.valueToString
                backendValue: backendValues.autoPlay
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Distance Model")
            tooltip: qsTr("Sets thow the volume of the sound scales with distance to the listener.")
        }

        SecondColumnLayout {
            ComboBox {
                scope: "SpatialSound"
                model: ["Logarithmic", "Linear", "ManualAttenuation"]
                backendValue: backendValues.distanceModel
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}

        }

        PropertyLabel {
            text: qsTr("Size")
            tooltip: qsTr("Set the size of the sound source.\nIf the listener is closer to the sound object than the size, volume will stay constant.")
        }

        SecondColumnLayout {
            SpinBox {
                minimumValue: 0
                maximumValue: 999999
                decimals: 2
                stepSize: 1
                backendValue: backendValues.size
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Distance Cutoff")
            tooltip: qsTr("Set the distance beyond which sound coming from the source will cutoff.")
        }

        SecondColumnLayout {
            SpinBox {
                minimumValue: 0
                maximumValue: 999999
                decimals: 2
                stepSize: 1
                backendValue: backendValues.distanceCutoff
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Manual Attenuation")
            tooltip: qsTr("Set the manual attenuation factor if distanceModel is set to ManualAttenuation.")
        }

        SecondColumnLayout {
            SpinBox {
                minimumValue: 0
                maximumValue: 999999
                decimals: 2
                stepSize: 0.1
                backendValue: backendValues.manualAttenuation
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Occlusion Intensity")
            tooltip: qsTr("Set how much the object is occluded.\n0 implies the object is not occluded at all, while a large number implies a large occlusion.")
        }

        SecondColumnLayout {
            SpinBox {
                minimumValue: 0
                maximumValue: 999999
                decimals: 2
                stepSize: 0.1
                backendValue: backendValues.occlusionIntensity
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Directivity")
            tooltip: qsTr("Set the directivity of the sound source.\nA value of 0 implies that the sound is emitted equally in all directions, while a value of 1 implies that the source mainly emits sound in the forward direction.")
        }

        SecondColumnLayout {
            SpinBox {
                minimumValue: 0
                maximumValue: 1
                decimals: 2
                stepSize: 0.01
                backendValue: backendValues.directivity
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Directivity Order")
            tooltip: qsTr("Set the order of the directivity of the sound source.\nA higher order implies a sharper localization of the sound cone.")
        }

        SecondColumnLayout {
            SpinBox {
                minimumValue: 1
                maximumValue: 999999
                decimals: 2
                stepSize: 1
                backendValue: backendValues.directivityOrder
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Near Field Gain")
            tooltip: qsTr("Set the near field gain for the sound source.\nA near field gain of 1 will raise the volume of the sound signal by approx 20 dB for distances very close to the listener.")
        }

        SecondColumnLayout {
            SpinBox {
                minimumValue: 0
                maximumValue: 1
                decimals: 2
                stepSize: 0.01
                backendValue: backendValues.nearFieldGain
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}
        }
    }
}
