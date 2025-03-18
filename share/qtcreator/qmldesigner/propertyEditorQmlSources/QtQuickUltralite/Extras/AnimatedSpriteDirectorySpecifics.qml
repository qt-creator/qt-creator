// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Layouts
import HelperWidgets
import StudioTheme as StudioTheme

Column {
    anchors.left: parent.left
    anchors.right: parent.right

    Section {
        anchors.left: parent.left
        anchors.right: parent.right

        caption: qsTr("Animated Sprite Directory")

        SectionLayout {
            PropertyLabel {
                text: qsTr("Current frame")
                tooltip: qsTr("Set this property to advance the current frame")
            }

            SecondColumnLayout {
                SpinBox {
                    backendValue: backendValues.currentFrame
                    minimumValue: 0
                    maximumValue: Number.MAX_VALUE
                    decimals: 0
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    enabled: backendValue.isAvailable
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Frame duration")
                tooltip: qsTr("Duration of each frame of the animation in milliseconds")
            }

            SecondColumnLayout {
                SpinBox {
                    backendValue: backendValues.frameDuration
                    minimumValue: 0
                    maximumValue: Number.MAX_VALUE
                    decimals: 0
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    enabled: backendValue.isAvailable
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Loops")
                tooltip: qsTr("Indicate the number of times the animation should reply, set to -1 for Infinite")
            }

            SecondColumnLayout {
                SpinBox {
                    backendValue: backendValues.loops
                    minimumValue: -1
                    maximumValue: Number.MAX_VALUE
                    decimals: 0
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    enabled: backendValue.isAvailable
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Running")
                tooltip: qsTr("Indicates whether the application is running or not")
            }

            SecondColumnLayout {
                CheckBox {
                    enabled: backendValues.running
                    text: backendValues.running.valueToString
                    backendValue: backendValues.running
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Paused")
                tooltip: qsTr("Indicate whether the animation is paused or not")
            }

            SecondColumnLayout {
                CheckBox {
                    enabled: backendValues.paused
                    text: backendValues.paused.valueToString
                    backendValue: backendValues.paused
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                }

                ExpandingSpacer {}
            }
            PropertyLabel {
                text: qsTr("Source path")
                tooltip: qsTr("Path to the directory with images for the sprite animation")
            }

            SecondColumnLayout {

                // QDS-11080: use UrlChooser instead when it supports folder selection
                LineEdit {
                    backendValue: backendValues.sourcePath
                    showTranslateCheckBox: false
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                }

                ExpandingSpacer {}
            }

        }
    }
}
