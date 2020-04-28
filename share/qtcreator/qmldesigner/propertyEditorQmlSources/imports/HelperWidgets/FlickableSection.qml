/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

import QtQuick 2.1
import HelperWidgets 2.0
import QtQuick.Layouts 1.0

Section {
    anchors.left: parent.left
    anchors.right: parent.right
    caption: qsTr("Flickable")

    id: root

    property int labelWidth: 42
    property int spinBoxWidth: 96

    SectionLayout {


        Label {
            text: qsTr("Flick direction")
        }

        SecondColumnLayout {
            ComboBox {
                backendValue: backendValues.flickableDirection
                model: ["AutoFlickDirection", "AutoFlickIfNeeded", "HorizontalFlick", "VerticalFlick", "HorizontalAndVerticalFlick"]
                Layout.fillWidth: true
                scope: "Flickable"
            }
            ExpandingSpacer {
            }
        }

        Label {
            text: qsTr("Behavior")
            tooltip: qsTr("Determines whether the surface may be dragged beyond the Flickable's boundaries, or overshoot the Flickable's boundaries when flicked.")
        }

        SecondColumnLayout {
            ComboBox {
                backendValue: backendValues.boundsBehavior
                model: ["StopAtBounds", "DragOverBounds", "OvershootBounds", "DragAndOvershootBounds"]
                Layout.fillWidth: true
                scope: "Flickable"
            }
            ExpandingSpacer {
            }
        }


        Label {
            text: qsTr("Movement")
            tooltip: qsTr("Determines whether the Flickable will give a feeling that the edges of the view are soft, rather than a hard physical boundary.")
        }

        SecondColumnLayout {
            ComboBox {
                backendValue: backendValues.boundsMovement
                model: ["FollowBoundsBehavior", "StopAtBounds"]
                Layout.fillWidth: true
                scope: "Flickable"
            }
            ExpandingSpacer {
            }
        }

        Label {
            text: qsTr("Interactive")
            tooltip: qsTr("Describes whether the user can interact with the Flickable. A user cannot drag or flick a Flickable that is not interactive.")
        }

        SecondColumnLayout {
            CheckBox {
                Layout.fillWidth: true
                backendValue: backendValues.interactive
                text: backendValues.interactive.valueToString
            }
            ExpandingSpacer {
            }
        }

        Label {
            text: qsTr("Max. velocity")
            tooltip: qsTr("Maximum flick velocity")
        }

        SecondColumnLayout {
            SpinBox {
                backendValue: backendValues.maximumFlickVelocity
                minimumValue: 0
                maximumValue: 8000
                decimals: 0
            }
            ExpandingSpacer {
            }
        }

        Label {
            text: qsTr("Deceleration")
            tooltip: qsTr("Flick deceleration")
        }

        SecondColumnLayout {
            SpinBox {
                backendValue: backendValues.flickDeceleration
                minimumValue: 0
                maximumValue: 8000
                decimals: 0
            }
            ExpandingSpacer {
            }
        }

        Label {
            text: qsTr("Press delay")
            tooltip: qsTr("Holds the time to delay (ms) delivering a press to children of the Flickable.")
        }

        SecondColumnLayout {
            SpinBox {
                backendValue: backendValues.pressDelay
                minimumValue: 0
                maximumValue: 2000
                decimals: 0
            }
            ExpandingSpacer {
            }
        }

        Label {
            text: qsTr("Pixel aligned")
            tooltip: qsTr("Sets the alignment of contentX and contentY to pixels (true) or subpixels (false).")
        }

        SecondColumnLayout {
            CheckBox {
                Layout.fillWidth: true
                backendValue: backendValues.pixelAligned
                text: backendValues.pixelAligned.valueToString
            }
            ExpandingSpacer {
            }
        }

        Label {
            text: qsTr("Synchronous drag")
            tooltip: qsTr("If set to true, then when the mouse or touchpoint moves far enough to begin dragging\n"
                          + "the content, the content will jump, such that the content pixel which was under the\n"
                          + "cursor or touchpoint when pressed remains under that point.")
        }

        SecondColumnLayout {
            CheckBox {
                Layout.fillWidth: true
                backendValue: backendValues.synchronousDrag
                text: backendValues.synchronousDrag.valueToString
            }
            ExpandingSpacer {
            }
        }

        Label {
            text: qsTr("Content size")
        }

        SecondColumnLayout {

            Label {
                text: "W"
                width: root.labelWidth
            }

            SpinBox {
                backendValue: backendValues.contentWidth
                minimumValue: 0
                maximumValue: 10000
                implicitWidth: root.spinBoxWidth
                Layout.fillWidth: true
            }

            Item {
                width: 4
                height: 4
            }

            Label {
                text: "H"
                width: root.labelWidth
            }

            SpinBox {
                backendValue: backendValues.contentHeight
                minimumValue: 0
                maximumValue: 10000
                implicitWidth: root.spinBoxWidth
                Layout.fillWidth: true
            }
            ExpandingSpacer {
            }
        }

        Label {
            text: qsTr("Content")
        }

        SecondColumnLayout {

            Label {
                text: "X"
                width: root.labelWidth
            }

            SpinBox {
                backendValue: backendValues.contentX
                minimumValue: -8000
                maximumValue: 8000
                implicitWidth: root.spinBoxWidth
                Layout.fillWidth: true
            }

            Item {
                width: 4
                height: 4
            }

            Label {
                text: "Y"
                width: root.labelWidth
            }

            SpinBox {
                backendValue: backendValues.contentY
                minimumValue: -8000
                maximumValue: 8000
                implicitWidth: root.spinBoxWidth
                Layout.fillWidth: true
            }
            ExpandingSpacer {
            }
        }

        Label {
            text: qsTr("Origin")
        }

        SecondColumnLayout {
            Label {
                text: "X"
                width: root.labelWidth
            }

            SpinBox {
                backendValue: backendValues.originX
                minimumValue: -8000
                maximumValue: 8000
                implicitWidth: root.spinBoxWidth
                Layout.fillWidth: true
            }

            Item {
                width: 4
                height: 4
            }

            Label {
                text: "Y"
                width: root.labelWidth
            }

            SpinBox {
                backendValue: backendValues.originY
                minimumValue: -8000
                maximumValue: 8000
                implicitWidth: root.spinBoxWidth
                Layout.fillWidth: true
            }
            ExpandingSpacer {
            }
        }

        Label {
            text: qsTr("Margins")
        }

        SecondColumnLayout {
            Layout.fillWidth: true

            Label {
                text: "Top"
                width: root.labelWidth
            }

            SpinBox {
                backendValue: backendValues.topMargin
                minimumValue: -10000
                maximumValue: 10000
                decimals: 0
                implicitWidth: root.spinBoxWidth
                Layout.fillWidth: true
            }

            Item {
                width: 4
                height: 4
            }

            Label {
                text: "Bottom"
                width: root.labelWidth
            }

            SpinBox {
                backendValue: backendValues.bottomMargin
                minimumValue: -10000
                maximumValue: 10000
                decimals: 0
                implicitWidth: root.spinBoxWidth
                Layout.fillWidth: true
            }
            ExpandingSpacer {
            }
        }

        Label {
            text: ("")
        }

        SecondColumnLayout {
            Layout.fillWidth: true

            Label {
                text: "Left"
                width: root.labelWidth
            }

            SpinBox {
                backendValue: backendValues.leftMargin
                minimumValue: -10000
                maximumValue: 10000
                decimals: 0
                implicitWidth: root.spinBoxWidth
                Layout.fillWidth: true
            }

            Item {
                width: 4
                height: 4
            }

            Label {
                text: "Right"
                width: root.labelWidth
            }

            SpinBox {
                backendValue: backendValues.rightMargin
                minimumValue: -10000
                maximumValue: 10000
                decimals: 0
                implicitWidth: root.spinBoxWidth
                Layout.fillWidth: true
            }
            ExpandingSpacer {
            }
        }
    }
}
