/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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
            toolTip: qsTr("Current value of the Slider. The default value is 0.0.")
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
            toolTip: qsTr(Maximum value of the slider. The default value is 1.0.")
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
            toolTip: qsTr("Minimum value of the slider. The default value is 0.0.")
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
            toolTip: qsTr("Layout orientation of the slider.")
        }
        SecondColumnLayout {
            OrientationCombobox {
            }
            ExpandingSpacer {

            }
        }
        Label {
            text: qsTr("Step size")
            toolTip: qsTr("Indicates the slider step size.")
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
            toolTip: qsTr("Indicates whether the slider should receive active focus when pressed.")
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
            toolTip: qsTr("Indicates whether the slider should display tick marks at step intervals.")
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
            toolTip: qsTr("Determines whether the current value should be updated while the user is moving the slider handle, or only when the button has been released.")
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
