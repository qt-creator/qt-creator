// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import QtQuick.Layouts 1.15
import HelperWidgets 2.0
import StudioControls 1.0 as StudioControls
import StudioTheme 1.0 as StudioTheme

Section {
    caption: qsTr("Video")

    anchors.left: parent.left
    anchors.right: parent.right

    SectionLayout {
        PropertyLabel { text: qsTr("Source") }

        SecondColumnLayout {
            UrlChooser {
                backendValue: backendValues.source
                filter: "*.mp4"
            }

            ExpandingSpacer {}
        }

        PropertyLabel { text: qsTr("Fill mode") }

        SecondColumnLayout {
            ComboBox {
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                width: implicitWidth
                scope: "VideoOutput"
                model: ["Stretch", "PreserveAspectFit", "PreserveAspectCrop"]
                backendValue: backendValues.fillMode
            }

            ExpandingSpacer {}
        }

        PropertyLabel { text: qsTr("Orientation") }

        SecondColumnLayout {
            ComboBox {
                id: orientationComboBox
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                width: implicitWidth
                model: [0, 90, 180, 270, 360]
                backendValue: backendValues.orientation
                manualMapping: true

                onValueFromBackendChanged: {
                    if (!orientationComboBox.__isCompleted)
                        return

                    orientationComboBox.syncIndexToBackendValue()
                }
                onCompressedActivated: {
                    if (!orientationComboBox.__isCompleted)
                        return

                    if (orientationComboBox.block)
                        return

                    backendValues.orientation.value = orientationComboBox.model[orientationComboBox.currentIndex]
                }
                Component.onCompleted: orientationComboBox.syncIndexToBackendValue()

                function syncIndexToBackendValue() {
                    orientationComboBox.block = true
                    orientationComboBox.currentIndex = orientationComboBox.model.indexOf(backendValues.orientation.value)
                    orientationComboBox.block = false
                }
            }

            ExpandingSpacer {}
        }
    }
}

