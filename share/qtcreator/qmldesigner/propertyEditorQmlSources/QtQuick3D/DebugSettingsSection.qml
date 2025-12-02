// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.15
import QtQuick.Layouts 1.15
import HelperWidgets 2.0
import StudioTheme 1.0 as StudioTheme

Section {
    caption: qsTr("Debug Settings")
    width: parent.width

    SectionLayout {
        PropertyLabel {
            text: qsTr("Enable Wireframe")
            tooltip: qsTr("Meshes will be rendered as wireframes.")
        }

        SecondColumnLayout {
            CheckBox {
                text: backendValues.wireframeEnabled.valueToString
                backendValue: backendValues.wireframeEnabled
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Override Mode")
            tooltip: qsTr("Changes how all materials are rendered to only reflect a particular aspect of the overall rendering process")
        }

        SecondColumnLayout {
            ComboBox {
                scope: "DebugSettings"
                model: ["None", "BaseColor", "Roughness", "Metalness", "Diffuse", "Specular", "ShadowOcclusion", "Emission", "AmbientOcclusion", "Normals", "Tangents", "Binormals", "FO"]
                backendValue: backendValues.materialOverride
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Shadow Boxes")
            tooltip: qsTr("A bounding box is drawn for every directional light's shadowmap.")
        }

        SecondColumnLayout {
            CheckBox {
                text: backendValues.drawDirectionalLightShadowBoxes.valueToString
                backendValue: backendValues.drawDirectionalLightShadowBoxes
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Shadow Cast Bounds")
            tooltip: qsTr("A bounding box is drawn for the shadow casting objects.")
        }

        SecondColumnLayout {
            CheckBox {
                text: backendValues.drawShadowCastingBounds.valueToString
                backendValue: backendValues.drawShadowCastingBounds
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Shadow Receive Bounds")
            tooltip: qsTr("A bounding box is drawn for the shadow receiving objects.")
        }

        SecondColumnLayout {
            CheckBox {
                text: backendValues.drawShadowReceivingBounds.valueToString
                backendValue: backendValues.drawShadowReceivingBounds
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Cascades")
            tooltip: qsTr("A frustum is drawn with splits indicating where the shadowmap cascades begin and end.")
        }

        SecondColumnLayout {
            CheckBox {
                text: backendValues.drawCascades.valueToString
                backendValue: backendValues.drawCascades
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Cascade Intersection")
            tooltip: qsTr("The intersection of the shadowmap cascades and the casting and receiving objects of the scene is drawn.")
        }

        SecondColumnLayout {
            CheckBox {
                text: backendValues.drawSceneCascadeIntersection.valueToString
                backendValue: backendValues.drawSceneCascadeIntersection
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Disable Shadow Camera")
            tooltip: qsTr("The camera update is disabled for the shadowmap.\nThis means that the view frustum will be locked in space just for the shadowmap calculations.")
        }

        SecondColumnLayout {
            CheckBox {
                text: backendValues.disableShadowCameraUpdate.valueToString
                backendValue: backendValues.disableShadowCameraUpdate
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}
        }
    }
}
