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

import QtQuick 2.0
import HelperWidgets 2.0
import QtQuick.Layouts 1.0

Section {
    anchors.left: parent.left
    anchors.right: parent.right
    caption: qsTr("Advanced")

    SectionLayout {
        rows: 4

        Label {
            text: qsTr("Origin")
            disabledState: !backendValues.transformOrigin.isAvailable
        }

        OriginControl {
            backendValue: backendValues.transformOrigin
            enabled: backendValues.transformOrigin.isAvailable
        }

        Label {
            text: qsTr("Scale")
            disabledState: !backendValues.scale.isAvailable
        }

        SecondColumnLayout {
            SpinBox {
                sliderIndicatorVisible: true
                backendValue: backendValues.scale
                hasSlider: true
                decimals: 2
                stepSize: 0.1
                minimumValue: -10
                maximumValue: 10
                Layout.preferredWidth: 140
                enabled: backendValues.scale.isAvailable
            }
            ExpandingSpacer {
            }
        }
        Label {
            text: qsTr("Rotation")
            disabledState: !backendValues.rotation.isAvailable
        }
        SecondColumnLayout {
            SpinBox {
                sliderIndicatorVisible: true
                backendValue: backendValues.rotation
                hasSlider: true
                decimals: 2
                minimumValue: -360
                maximumValue: 360
                Layout.preferredWidth: 140
                enabled: backendValues.rotation.isAvailable
            }
            ExpandingSpacer {
            }
        }
        Label {
            text: "Z"
        }
        SecondColumnLayout {
            SpinBox {
                sliderIndicatorVisible: true
                backendValue: backendValues.z
                hasSlider: true
                minimumValue: -100
                maximumValue: 100
                Layout.preferredWidth: 140
            }
            ExpandingSpacer {
            }
        }

        Label {
            text: qsTr("State")
        }
        SecondColumnLayout {

            ComboBox {
                Layout.fillWidth: true
                backendValue: backendValues.state
                model: allStateNames
                valueType: ComboBox.String
            }

            ExpandingSpacer {
            }
        }

        Label {
            visible: majorQtQuickVersion > 1
            text: qsTr("Enabled")
        }
        SecondColumnLayout {
            visible: majorQtQuickVersion > 1
            CheckBox {
                backendValue: backendValues.enabled
                text: qsTr("Accept mouse and keyboard events")
            }
            ExpandingSpacer {
            }
        }

        Label {
            visible: majorQtQuickVersion > 1
            text: qsTr("Smooth")
            disabledState: !backendValues.smooth.isAvailable
        }
        SecondColumnLayout {
            visible: majorQtQuickVersion > 1
            CheckBox {
                backendValue: backendValues.smooth
                text: qsTr("Smooth sampling active")
                enabled: backendValues.smooth.isAvailable
            }
            ExpandingSpacer {
            }
        }

        Label {
            visible: majorQtQuickVersion > 1
            text: qsTr("Antialiasing")
            disabledState: !backendValues.antialiasing.isAvailable
        }
        SecondColumnLayout {
            visible: majorQtQuickVersion > 1
            CheckBox {
                backendValue: backendValues.antialiasing
                text: qsTr("Anti-aliasing active")
                enabled: backendValues.antialiasing.isAvailable
            }
            ExpandingSpacer {
            }
        }

        Label {
            text: qsTr("Focus")
            tooltip: qsTr("Holds whether the item has focus within the enclosing FocusScope.")
            disabledState: !backendValues.focus.isAvailable
        }
        SecondColumnLayout {
            CheckBox {
                backendValue: backendValues.focus
                text: backendValues.focus.valueToString
                enabled: backendValues.focus.isAvailable
                implicitWidth: 180
            }
            ExpandingSpacer {
            }
        }

        Label {
            text: qsTr("Active focus on tab")
            tooltip: qsTr("Holds whether the item wants to be in the tab focus chain.")
            disabledState: !backendValues.activeFocusOnTab.isAvailable
        }
        SecondColumnLayout {
            CheckBox {
                backendValue: backendValues.activeFocusOnTab
                text: backendValues.activeFocusOnTab.valueToString
                enabled: backendValues.activeFocusOnTab.isAvailable
                implicitWidth: 180
            }
            ExpandingSpacer {
            }
        }

        Label {
            text: qsTr("Baseline offset")
            tooltip: qsTr("Specifies the position of the item's baseline in local coordinates.")
            disabledState: !backendValues.baselineOffset.isAvailable
        }
        SecondColumnLayout {
            SpinBox {
                sliderIndicatorVisible: true
                backendValue: backendValues.baselineOffset
                hasSlider: true
                decimals: 0
                minimumValue: -1000
                maximumValue: 1000
                Layout.preferredWidth: 140
                enabled: backendValues.baselineOffset.isAvailable
            }
            ExpandingSpacer {
            }
        }
    }
}
