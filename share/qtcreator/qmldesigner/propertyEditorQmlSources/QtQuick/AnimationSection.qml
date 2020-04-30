/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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
import StudioTheme 1.0 as StudioTheme

Section {
    id: section
    caption: qsTr("Animation")
    anchors.left: parent.left
    anchors.right: parent.right

    property bool showDuration: true
    property bool showEasingCurve: false

    SectionLayout {
        Label {
            text: qsTr("Running")
            tooltip: qsTr("Sets whether the animation is currently running.")
        }

        CheckBox {
            text: backendValues.running.valueToString
            backendValue: backendValues.running
        }

        Label {
            text: qsTr("Paused")
            tooltip: qsTr("Sets whether the animation is currently paused.")
        }

        CheckBox {
            text: backendValues.paused.valueToString
            backendValue: backendValues.paused
        }
        Label {
            text: qsTr("Loops")
            tooltip: qsTr("Sets the number of times the animation should play.")
        }

        SecondColumnLayout {
            SpinBox {
                maximumValue: 9999999
                minimumValue: -1
                backendValue: backendValues.loops
                Layout.fillWidth: true
                Layout.maximumWidth: 100
            }

            ExpandingSpacer {
            }
        }

        Label {
            visible: section.showDuration
            text: qsTr("Duration")
            tooltip: qsTr("Sets the duration of the animation, in milliseconds.")
        }

        SecondColumnLayout {
            visible: section.showDuration
            SpinBox {
                maximumValue: 9999999
                minimumValue: -9999999
                backendValue: backendValues.duration
                Layout.fillWidth: true
                Layout.maximumWidth: 100
            }

            ExpandingSpacer {
            }
        }
        Label {
            text: qsTr("Always Run To End")
            tooltip: qsTr("Sets whether the animation should run to completion when it is stopped.")
        }

        CheckBox {
            text: backendValues.alwaysRunToEnd.valueToString
            backendValue: backendValues.alwaysRunToEnd
        }

        Label {
            visible: section.showEasingCurve
            text: qsTr("Easing Curve")
            tooltip: qsTr("Define custom easing curve")
        }

        BoolButtonRowButton {
            visible: section.showEasingCurve
            buttonIcon: StudioTheme.Constants.curveDesigner
            EasingCurveEditor {
                id: easingCurveEditor
                modelNodeBackendProperty: modelNodeBackend
            }
            onClicked: easingCurveEditor.runDialog()

        }
    }
}
