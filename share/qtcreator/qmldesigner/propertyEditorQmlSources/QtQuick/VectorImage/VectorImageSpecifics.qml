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
        caption: qsTr("VectorImage")

        anchors.left: parent.left
        anchors.right: parent.right

        SectionLayout {
            PropertyLabel {
                text: qsTr("Source")
                tooltip: qsTr("Adds a vector image from your device.")
            }

            SecondColumnLayout {
                UrlChooser {
                    backendValue: backendValues.source
                    filter: "*.svg *.svgz"
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Fill mode")
                tooltip: qsTr("Sets how the vector image fits in the content box.")
            }

            SecondColumnLayout {
                ComboBox {
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    width: implicitWidth
                    scope: "VectorImage"
                    model: ["NoResize", "Stretch", "PreserveAspectFit", "PreserveAspectCrop"]
                    backendValue: backendValues.fillMode
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Preferred renderer type")
                tooltip: qsTr("Sets how the vector shapes in the image are rendered.")
            }

            SecondColumnLayout {
                ComboBox {
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    width: implicitWidth
                    scope: "VectorImage"
                    model: ["GeometryRenderer", "CurveRenderer"]
                    backendValue: backendValues.preferredRendererType
                }

                ExpandingSpacer {}
            }
        }
    }
}
