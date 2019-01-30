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

    property int spinBoxWidth: 62

    SectionLayout {


        Label {
            text: qsTr("Flick direction")
        }

        SecondColumnLayout {
            ComboBox {
                backendValue: backendValues.flickableDirection
                model: ["AutoFlickDirection", "HorizontalFlick", "VerticalFlick", "HorizontalAndVerticalFlick"]
                Layout.fillWidth: true
                scope: "Flickable"
            }

        }

        Label {
            text: qsTr("Behavior")
            tooltip: qsTr("Determines whether the surface may be dragged beyond the Flickable's boundaries, or overshoot the Flickable's boundaries when flicked.")
        }

        SecondColumnLayout {
            ComboBox {
                backendValue: backendValues.boundsBehavior
                model: ["StopAtBounds", "DragOverBounds", "DragAndOvershootBounds"]
                Layout.fillWidth: true
                scope: "Flickable"
            }

        }


        Label {
            text: qsTr("Movement")
            tooltip: qsTr("Determines whether the flickable will give a feeling that the edges of the view are soft, rather than a hard physical boundary.")
        }

        SecondColumnLayout {
            ComboBox {
                backendValue: backendValues.boundsMovement
                model: ["FollowBoundsBehavior", "StopAtBounds"]
                Layout.fillWidth: true
                scope: "Flickable"
            }

        }

        Label {
            text:qsTr("Interactive")
        }

        SecondColumnLayout {
            CheckBox {
                backendValue: backendValues.interactive
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
            text:qsTr("Pixel aligned")
        }

        SecondColumnLayout {
            CheckBox {
                backendValue: backendValues.pixelAligned
                tooltip: qsTr("Sets the alignment of contentX and contentY to pixels (true) or subpixels (false).")
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
                width: 28
            }

            SpinBox {
                backendValue: backendValues.contentWidth
                minimumValue: 0
                maximumValue: 8000
                implicitWidth: root.spinBoxWidth
            }

            Item {
                width: 4
                height: 4
            }

            Label {
                text: "H"
                width: 28
            }

            SpinBox {
                backendValue: backendValues.contentHeight
                minimumValue: 0
                maximumValue: 8000
                implicitWidth: root.spinBoxWidth

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
                width: 28
            }

            SpinBox {
                backendValue: backendValues.contentX
                minimumValue: 0
                maximumValue: 8000
                implicitWidth: root.spinBoxWidth
            }

            Item {
                width: 4
                height: 4
            }

            Label {
                text: "Y"
                width: 28
            }

            SpinBox {
                backendValue: backendValues.contentY
                minimumValue: 0
                maximumValue: 8000
                implicitWidth: root.spinBoxWidth

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
                width: 28
            }

            SpinBox {
                backendValue: backendValues.topMargin
                maximumValue: 0xffff
                minimumValue: 0
                decimals: 0
                implicitWidth: root.spinBoxWidth
            }

            Item {
                width: 4
                height: 4
            }

            Label {
                text: "Bottom"
                width: 28
            }

            SpinBox {
                backendValue: backendValues.bottomMargin
                maximumValue: 0xffff
                minimumValue: 0
                decimals: 0
                implicitWidth: root.spinBoxWidth
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
                width: 28
            }

            SpinBox {
                backendValue: backendValues.leftMargin
                maximumValue: 0xffff
                minimumValue: 0
                decimals: 0
                implicitWidth: root.spinBoxWidth
            }

            Item {
                width: 4
                height: 4
            }

            Label {
                text: "Right"
                width: 28
            }

            SpinBox {
                backendValue: backendValues.rightMargin
                maximumValue: 0xffff
                minimumValue: 0
                decimals: 0
                implicitWidth: root.spinBoxWidth
            }
            ExpandingSpacer {

            }
        }

    }
}
