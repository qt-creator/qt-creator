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

