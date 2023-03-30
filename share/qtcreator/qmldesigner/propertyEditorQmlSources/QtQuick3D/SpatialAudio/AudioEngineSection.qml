// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0 WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import QtQuick.Layouts 1.15
import HelperWidgets 2.0
import StudioTheme 1.0 as StudioTheme

Section {
    caption: qsTr("Audio Engine")
    width: parent.width

    SectionLayout {
        PropertyLabel {
            text: qsTr("Master Volume")
            tooltip: qsTr("Sets or returns overall volume being used to render the sound field.\nValues between 0 and 1 will attenuate the sound, while values above 1 provide an additional gain boost.")
        }

        SecondColumnLayout {
            SpinBox {
                minimumValue: 0
                maximumValue: 999999
                decimals: 2
                stepSize: 0.1
                backendValue: backendValues.masterVolume
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Output Mode")
            tooltip: qsTr("Sets the current output mode of the engine.")
        }

        SecondColumnLayout {
            ComboBox {
                scope: "AudioEngine"
                model: ["Surround", "Stereo", "Headphone"]
                backendValue: backendValues.outputMode
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Output Device")
            tooltip: qsTr("Sets the device that is being used for outputting the sound field.")
        }

        SecondColumnLayout {
            ItemFilterComboBox {
                typeFilter: "QtMultimedia.AudioDevice"
                backendValue: backendValues.outputDevice
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}
        }
    }
}
