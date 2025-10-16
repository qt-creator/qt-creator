// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import HelperWidgets
import StudioTheme as StudioTheme

Column {
    anchors.left: parent.left
    anchors.right: parent.right

    Section {
        caption: qsTr("ShaderEffect")
        anchors.left: parent.left
        anchors.right: parent.right

        SectionLayout {
            PropertyLabel {
                text: qsTr("Fragment Shader")
                tooltip: qsTr("Sets the location of the fragment shader.")
            }

            SecondColumnLayout {
                UrlChooser {
                    filter: "*.qsb"
                    backendValue: backendValues.fragmentShader
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Vertex Shader")
                tooltip: qsTr("Sets the location of the vertex shader.")
            }

            SecondColumnLayout {
                UrlChooser {
                    filter: "*.qsb"
                    backendValue: backendValues.vertexShader
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Atlas Textures")
                tooltip: qsTr("Set to true to indicate atlas textures are supported by this shader effect.")
            }

            SecondColumnLayout {
                CheckBox {
                    text: backendValues.supportsAtlasTextures.valueToString
                    backendValue: backendValues.supportsAtlasTextures
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Blending")
                tooltip: qsTr("Enables blending of output from fragment shader with the background using source-over blend mode.")
            }

            SecondColumnLayout {
                CheckBox {
                    text: backendValues.blending.valueToString
                    backendValue: backendValues.blending
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Cull Mode")
                tooltip: qsTr("Sets what faces of meshes are culled.")
            }

            SecondColumnLayout {
                ComboBox {
                    scope: "ShaderEffect"
                    model: ["NoCulling", "BackFaceCulling", "FrontFaceCulling"]
                    backendValue: backendValues.cullMode
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Grid Mesh")
                tooltip: qsTr("Sets the grid mesh to use.")
            }

            SecondColumnLayout {
                ItemFilterComboBox {
                    typeFilter: "QtQuick.GridMesh"
                    backendValue: backendValues.mesh
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                }

                ExpandingSpacer {}
            }
        }
    }
}
