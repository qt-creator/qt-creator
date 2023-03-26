// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import QtQuick.Layouts 1.15
import HelperWidgets 2.0
import StudioControls 1.0 as StudioControls
import StudioTheme 1.0 as StudioTheme

Column {
    anchors.left: parent.left
    anchors.right: parent.right

    ImageSection {
        caption: qsTr("Image")
    }

    Section {
        caption: qsTr("Animated image")

        anchors.left: parent.left
        anchors.right: parent.right

        SectionLayout {
            PropertyLabel {
                text: qsTr("Speed")
                tooltip: qsTr("Sets the speed of the animation.")
                blockedByTemplate: !backendValues.speed.isAvailable
            }

            SecondColumnLayout {
                SpinBox {
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    sliderIndicatorVisible: true
                    backendValue: backendValues.speed
                    hasSlider: true
                    decimals: 2
                    minimumValue: 0
                    maximumValue: 100
                    enabled: backendValues.speed.isAvailable
                }

                // TODO convert to % and add % label after the spin box

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Playing")
                tooltip: qsTr("Toggles if the animation is playing.")
                blockedByTemplate: !backendValues.playing.isAvailable && !backendValues.paused.isAvailable
            }

            SecondColumnLayout {
                CheckBox {
                    text: StudioTheme.Constants.play
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                + StudioTheme.Values.actionIndicatorWidth
                    backendValue: backendValues.playing
                    enabled: backendValue.isAvailable
                    fontFamily: StudioTheme.Constants.iconFont.family
                    fontPixelSize: StudioTheme.Values.myIconFontSize
                }

                Spacer { implicitWidth: StudioTheme.Values.twoControlColumnGap }

                CheckBox {
                    text: StudioTheme.Constants.pause
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                + StudioTheme.Values.actionIndicatorWidth
                    backendValue: backendValues.paused
                    enabled: backendValue.isAvailable
                    fontFamily: StudioTheme.Constants.iconFont.family
                    fontPixelSize: StudioTheme.Values.myIconFontSize
                }

                ExpandingSpacer {}
            }
        }
    }
}
