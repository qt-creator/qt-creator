// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import HelperWidgets 2.0
import QtQuick.Layouts 1.15
import StudioControls 1.0 as StudioControls
import StudioTheme 1.0 as StudioTheme

Section {
    id: root
    caption: qsTr("Flickable")
    anchors.left: parent.left
    anchors.right: parent.right

    SectionLayout {
        PropertyLabel {
            text: qsTr("Interactive")
            tooltip: qsTr("Toggles if the flickable supports drag and flick actions.")
        }

        SecondColumnLayout {
            CheckBox {
                text: backendValues.interactive.valueToString
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.interactive
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Flick direction")
            tooltip: qsTr("Sets which directions the view can be flicked.")
            blockedByTemplate: !backendValues.flickableDirection.isAvailable
        }

        SecondColumnLayout {
            ComboBox {
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                width: implicitWidth
                backendValue: backendValues.flickableDirection
                model: ["AutoFlickDirection", "AutoFlickIfNeeded", "HorizontalFlick", "VerticalFlick", "HorizontalAndVerticalFlick"]
                scope: "Flickable"
                enabled: backendValue.isAvailable
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Behavior")
            tooltip: qsTr("Sets how the flickable behaves when it is dragged beyond its boundaries.")
            blockedByTemplate: !backendValues.boundsBehavior.isAvailable
        }

        SecondColumnLayout {
            ComboBox {
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                width: implicitWidth
                backendValue: backendValues.boundsBehavior
                model: ["StopAtBounds", "DragOverBounds", "OvershootBounds", "DragAndOvershootBounds"]
                scope: "Flickable"
                enabled: backendValue.isAvailable
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Movement")
            tooltip: qsTr("Sets if the edges of the flickable should be soft or hard.")
            blockedByTemplate: !backendValues.boundsMovement.isAvailable
        }

        SecondColumnLayout {
            ComboBox {
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                width: implicitWidth
                backendValue: backendValues.boundsMovement
                model: ["FollowBoundsBehavior", "StopAtBounds"]
                scope: "Flickable"
                enabled: backendValue.isAvailable
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Max. velocity")
            tooltip: qsTr("Sets how fast an item can be flicked.")
        }

        SecondColumnLayout {
            SpinBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.maximumFlickVelocity
                minimumValue: 0
                maximumValue: 8000
                decimals: 0
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Deceleration")
            tooltip: qsTr("Sets the rate by which a flick should slow down.")
            blockedByTemplate: !backendValues.flickDeceleration.isAvailable
        }

        SecondColumnLayout {
            SpinBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.flickDeceleration
                minimumValue: 0
                maximumValue: 8000
                decimals: 0
                enabled: backendValue.isAvailable
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Press delay")
            tooltip: qsTr("Sets the time to delay delivering a press to children of the flickable in milliseconds.")
            blockedByTemplate: !backendValues.pressDelay.isAvailable
        }

        SecondColumnLayout {
            SpinBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.pressDelay
                minimumValue: 0
                maximumValue: 2000
                decimals: 0
                enabled: backendValue.isAvailable
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Pixel aligned")
            tooltip: qsTr("Toggles if the component is being moved by complete pixel length.")
            blockedByTemplate: !backendValues.pixelAligned.isAvailable
        }

        SecondColumnLayout {
            CheckBox {
                text: backendValues.pixelAligned.valueToString
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.pixelAligned
                enabled: backendValue.isAvailable
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Synchronous drag")
            tooltip: qsTr("Toggles if the content should move instantly or not when the mouse or touchpoint is dragged to a new position.")
            blockedByTemplate: !backendValues.synchronousDrag.isAvailable
        }

        SecondColumnLayout {
            CheckBox {
                text: backendValues.synchronousDrag.valueToString
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.synchronousDrag
                enabled: backendValue.isAvailable
            }

            ExpandingSpacer {}
        }

    }
}
