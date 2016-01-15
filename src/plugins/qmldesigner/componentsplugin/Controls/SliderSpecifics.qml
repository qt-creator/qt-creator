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

import HelperWidgets 2.0
import QtQuick 2.1
import QtQuick.Layouts 1.1
Section {
    caption: "Slider"
    SectionLayout {
        Label {
            text: qsTr("Value")
            tooltip: qsTr("Current value of the Slider. The default value is 0.0.")
        }
        SecondColumnLayout {
            SpinBox {
                maximumValue: backendValues.maximumValue.value
                minimumValue: backendValues.minimumValue.value
                decimals: 2
                stepSize: backendValues.stepSize.value
                backendValue: backendValues.value
                implicitWidth: 180
            }
            ExpandingSpacer {}
        }
        Label {
            text: qsTr("Maximum value")
            tooltip: qsTr("Maximum value of the slider. The default value is 1.0.")
        }
        SecondColumnLayout {
            SpinBox {
                maximumValue: 9999999
                minimumValue: -9999999
                decimals: 2
                backendValue: backendValues.maximumValue
                implicitWidth: 180
            }
            ExpandingSpacer {

            }
        }
        Label {
            text: qsTr("Minimum value")
            tooltip: qsTr("Minimum value of the slider. The default value is 0.0.")
        }
        SecondColumnLayout {
            SpinBox {
                maximumValue: 9999999
                minimumValue: -9999999
                decimals: 2
                backendValue: backendValues.minimumValue
                implicitWidth: 180
            }
            ExpandingSpacer {

            }
        }
        Label {
            text: qsTr("Orientation")
            tooltip: qsTr("Layout orientation of the slider.")
        }
        SecondColumnLayout {
            OrientationCombobox {
            }
            ExpandingSpacer {

            }
        }
        Label {
            text: qsTr("Step size")
            tooltip: qsTr("Indicates the slider step size.")
        }
        SecondColumnLayout {
            SpinBox {
                maximumValue: 9999999
                minimumValue: -9999999
                decimals: 2
                backendValue: backendValues.stepSize
                implicitWidth: 180
            }
            ExpandingSpacer {}
        }

        Label {
            text: qsTr("Active focus on press")
            tooltip: qsTr("Indicates whether the slider should receive active focus when pressed.")
        }

        SecondColumnLayout {
            CheckBox {
                text: backendValues.activeFocusOnPress.valueToString
                backendValue: backendValues.activeFocusOnPress
                implicitWidth: 180
            }
            ExpandingSpacer {}
        }
        Label {
            text: qsTr("Tick marks enabled")
            tooltip: qsTr("Indicates whether the slider should display tick marks at step intervals.")
        }

        SecondColumnLayout {
            CheckBox {
                text: backendValues.tickmarksEnabled.valueToString
                backendValue: backendValues.tickmarksEnabled
                implicitWidth: 180
            }
            ExpandingSpacer {}
        }
        Label {
            text: qsTr("Update value while dragging")
            tooltip: qsTr("Determines whether the current value should be updated while the user is moving the slider handle, or only when the button has been released.")
        }

        SecondColumnLayout {
            CheckBox {
                text: backendValues.updateValueWhileDragging.valueToString
                backendValue: backendValues.updateValueWhileDragging
                implicitWidth: 180
            }
        }
    }
}
