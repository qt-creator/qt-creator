/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

import QtQuick 2.15
import HelperWidgets 2.0
import QtQuick.Layouts 1.15
import StudioControls 1.0 as StudioControls
import StudioTheme 1.0 as StudioTheme

Section {
    id: section
    caption: qsTr("Animation")

    anchors.left: parent.left
    anchors.right: parent.right

    property bool showDuration: true
    property bool showEasingCurve: false

    SectionLayout {
        PropertyLabel {
            text: qsTr("Running")
            tooltip: qsTr("Whether the animation is running and/or paused.")
        }

        SecondColumnLayout {
            CheckBox {
                text: StudioTheme.Constants.play
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.running
                enabled: backendValue.isAvailable
                fontFamily: StudioTheme.Constants.iconFont.family
                fontPixelSize: StudioTheme.Values.myIconFontSize
            }

            Spacer { implicitWidth: StudioTheme.Values.twoControlColumnGap }

            CheckBox {
                text: StudioTheme.Constants.pause
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.paused
                enabled: backendValue.isAvailable
                fontFamily: StudioTheme.Constants.iconFont.family
                fontPixelSize: StudioTheme.Values.myIconFontSize
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Loops")
            tooltip: qsTr("Number of times the animation should play.")
        }

        SecondColumnLayout {
            SpinBox {
                id: loopSpinBox
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.loops
                maximumValue: 9999999
                minimumValue: -1
            }

            Spacer { implicitWidth: StudioTheme.Values.controlGap }

            StudioControls.InfinityLoopIndicator {
                id: infinityLoopIndicator

                property var valueFromBackend: backendValues.loops.value

                onValueFromBackendChanged: {
                    if (valueFromBackend === -1)
                        infinityLoopIndicator.infinite = true
                    else
                        infinityLoopIndicator.infinite = false
                }

                onInfiniteChanged: {
                    if (infinityLoopIndicator.infinite === true)
                        backendValues.loops.value = -1
                    else
                        backendValues.loops.value = ((backendValues.loops.value < 0) ? 1 : backendValues.loops.value)
                }
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Duration")
            tooltip: qsTr("Duration of the animation in milliseconds.")
            visible: section.showDuration
        }

        SecondColumnLayout {
            visible: section.showDuration

            SpinBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                maximumValue: 9999999
                minimumValue: -9999999
                backendValue: backendValues.duration
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Run to end")
            tooltip: qsTr("Runs the animation to completion when it is stopped.")
        }

        SecondColumnLayout {
            CheckBox {
                text: backendValues.alwaysRunToEnd.valueToString
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.alwaysRunToEnd
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Easing curve")
            tooltip: qsTr("Defines a custom easing curve.")
            visible: section.showEasingCurve
        }

        SecondColumnLayout {
            BoolButtonRowButton {
                visible: section.showEasingCurve
                buttonIcon: StudioTheme.Constants.curveDesigner

                EasingCurveEditor {
                    id: easingCurveEditor
                    modelNodeBackendProperty: modelNodeBackend
                }

                onClicked: easingCurveEditor.runDialog()
            }

            ExpandingSpacer {}
        }
    }
}
