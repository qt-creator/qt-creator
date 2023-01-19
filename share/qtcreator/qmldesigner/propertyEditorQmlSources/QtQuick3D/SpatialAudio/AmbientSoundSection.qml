// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0 WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import QtQuick.Layouts 1.15
import HelperWidgets 2.0
import StudioTheme 1.0 as StudioTheme

Section {
    caption: qsTr("Ambient Sound")
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
            tooltip: qsTr("Sets how often the sound is played before the player stops.\nBind to AmbientSound.Infinite to loop the current sound forever.")
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
    }
}
