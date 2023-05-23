/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

import QtQuick
import QtQuick.Layouts
import HelperWidgets 2.0
import StudioControls 1.0 as StudioControls
import StudioTheme 1.0 as StudioTheme

Column {
    anchors.left: parent.left
    anchors.right: parent.right

    Section {
        caption: qsTr("Timer")

        anchors.left: parent.left
        anchors.right: parent.right

        SectionLayout {
            PropertyLabel {
                text: qsTr("Interval")
                tooltip: qsTr("Sets the interval between triggers, in milliseconds.")
            }

            SecondColumnLayout {
                SpinBox {
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    minimumValue: 0
                    maximumValue: 9999999
                    backendValue: backendValues.interval
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Repeat")
                tooltip: qsTr("Sets whether the timer is triggered repeatedly at the specified interval or just once.")
            }

            SecondColumnLayout {
                CheckBox {
                    text: backendValues.repeat.valueToString
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    backendValue: backendValues.repeat
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Running")
                tooltip: qsTr("Sets whether the timer is running or not.")
            }

            SecondColumnLayout {
                CheckBox {
                    text: backendValues.running.valueToString
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    backendValue: backendValues.running
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Triggered on start")
                tooltip: qsTr("Sets the timer to trigger when started.")
                blockedByTemplate: !triggeredOnStartComboBox.enabled
            }

            SecondColumnLayout {
                CheckBox {
                    id: triggeredOnStartComboBox
                    text: backendValues.triggeredOnStart.valueToString
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    backendValue: backendValues.triggeredOnStart
                    enabled: backendValues.triggeredOnStart.isAvailable
                }

                ExpandingSpacer {}
            }
        }
    }
}
