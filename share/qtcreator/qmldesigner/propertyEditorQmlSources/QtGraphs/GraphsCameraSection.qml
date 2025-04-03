// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import HelperWidgets
import QtQuick.Layouts
import StudioTheme as StudioTheme

Section {
    width: parent.width
    caption: qsTr("Camera")

    SectionLayout {
        PropertyLabel {
            text: qsTr("Preset")
            tooltip: qsTr("Camera preset")
        }
        SecondColumnLayout {
            ComboBox {
                backendValue: backendValues.cameraPreset
                model: ["NoPreset", "FrontLow", "Front", "FrontHigh", "LeftLow",
                    "Left", "LeftHigh", "RightLow", "Right", "RightHigh", "BehindLow",
                    "Behind", "BehindHigh", "IsometricLeft", "IsometricLeftHigh",
                    "IsometricRight", "IsometricRightHigh", "DirectlyAbove",
                    "DirectlyAboveCW45", "DirectlyAboveCCW45", "FrontBelow",
                    "LeftBelow", "RightBelow", "BehindBelow", "DirectlyBelow"]
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                scope: "Graphs3D"
            }
        }
        PropertyLabel {
            text: qsTr("Target")
            tooltip: qsTr("Camera target position")
        }
        SecondColumnLayout {
            SpinBox {
                backendValue: backendValues.cameraTargetPosition_x
                minimumValue: -1.0
                maximumValue: 1.0
                stepSize: 0.01
                decimals: 2
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
            }
            ControlLabel {
                text: "X"
                width: StudioTheme.Values.actionIndicatorWidth
            }
        }
        PropertyLabel {}
        SecondColumnLayout {
            SpinBox {
                backendValue: backendValues.cameraTargetPosition_y
                minimumValue: -1.0
                maximumValue: 1.0
                stepSize: 0.01
                decimals: 2
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
            }
            ControlLabel {
                text:"Y"
                width: StudioTheme.Values.actionIndicatorWidth
            }
        }
        PropertyLabel {}
        SecondColumnLayout {
            SpinBox {
                backendValue: backendValues.cameraTargetPosition_z
                minimumValue: -1.0
                maximumValue: 1.0
                stepSize: 0.01
                decimals: 2
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
            }
            ControlLabel {
                text: "Z"
                width: StudioTheme.Values.actionIndicatorWidth
            }
        }
        PropertyLabel {
            text: qsTr("Zoom")
            tooltip: qsTr("Camera zoom level")
        }
        SecondColumnLayout {
            SpinBox {
                backendValue: backendValues.cameraZoomLevel
                minimumValue: 0
                maximumValue: 500
                stepSize: 1
                decimals: 0
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }
        }
        PropertyLabel {
            text: qsTr("Min Zoom")
            tooltip: qsTr("Camera minimum zoom")
        }
        SecondColumnLayout {
            SpinBox {
                backendValue: backendValues.minCameraZoomLevel
                minimumValue: 0
                maximumValue: 500
                stepSize: 1
                decimals: 0
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }
        }
        PropertyLabel {
            text: qsTr("Max Zoom")
            tooltip: qsTr("Camera maximum zoom")
        }
        SecondColumnLayout {
            SpinBox {
                backendValue: backendValues.maxCameraZoomLevel
                minimumValue: 0
                maximumValue: 500
                stepSize: 1
                decimals: 0
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }
        }
        PropertyLabel {
            text: qsTr("X Rotation")
            tooltip: qsTr("Camera X rotation")
        }
        SecondColumnLayout {
            SpinBox {
                backendValue: backendValues.cameraXRotation
                minimumValue: -180
                maximumValue: 180
                stepSize: 1
                decimals: 0
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }
        }
        PropertyLabel {
            text: qsTr("Wrap X")
            tooltip: qsTr("Wrap camera X rotation")
        }
        SecondColumnLayout {
            CheckBox {
                backendValue: backendValues.wrapCameraXRotation
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }
        }
        PropertyLabel {
            text: qsTr("Y Rotation")
            tooltip: qsTr("Camera Y rotation")
        }
        SecondColumnLayout {
            SpinBox {
                backendValue: backendValues.cameraYRotation
                minimumValue: 0
                maximumValue: 90
                stepSize: 1
                decimals: 0
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }
        }
        PropertyLabel {
            text: qsTr("Wrap Y")
            tooltip: qsTr("Wrap camera Y rotation")
        }
        SecondColumnLayout {
            CheckBox {
                backendValue: backendValues.wrapCameraYRotation
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }
        }
        PropertyLabel {
            text: qsTr("Orthographic")
            tooltip: qsTr("Use orthographic camera")
        }
        SecondColumnLayout {
            CheckBox {
                backendValue: backendValues.orthoProjection
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }
        }
    }
}
