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

import QtQuick 2.15
import QtQuick.Layouts 1.15
import HelperWidgets 2.0
import StudioControls 1.0 as StudioControls
import StudioTheme 1.0 as StudioTheme

Section {
    caption: qsTr("Geometry - 2d")

    anchors.left: parent.left
    anchors.right: parent.right

    SectionLayout {
        PropertyLabel { text: qsTr("Position") }

        SecondColumnLayout {
            SpinBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.x
                maximumValue: 0xffff
                minimumValue: -0xffff
                decimals: 0
            }

            Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

            ControlLabel { text: "X" }

            Spacer { implicitWidth: StudioTheme.Values.controlGap }

            SpinBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.y
                maximumValue: 0xffff
                minimumValue: -0xffff
                decimals: 0
            }

            Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

            ControlLabel { text: "Y" }

            Spacer { implicitWidth: StudioTheme.Values.controlGap }

            LinkIndicator2D {}

            ExpandingSpacer {}
        }

        PropertyLabel { text: qsTr("Size") }

        SecondColumnLayout {
            SpinBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.width
                maximumValue: 0xffff
                minimumValue: 0
                decimals: 0
            }

            Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

            ControlLabel { text: qsTr("W") }

            Spacer { implicitWidth: StudioTheme.Values.controlGap }

            SpinBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.height
                maximumValue: 0xffff
                minimumValue: 0
                decimals: 0
            }

            Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

            ControlLabel { text: qsTr("H") }

            Spacer { implicitWidth: StudioTheme.Values.controlGap }

            LinkIndicator2D {}

            ExpandingSpacer {}
        }

        PropertyLabel { text: qsTr("Rotation") }

        SecondColumnLayout {
            SpinBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.rotation
                decimals: 2
                minimumValue: -360
                maximumValue: 360
                enabled: backendValues.rotation.isAvailable
            }

            Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

            ControlLabel { text: "°" }

            Spacer { implicitWidth: StudioTheme.Values.controlGap }

            StudioControls.ButtonRow {
                actionIndicatorVisible: true

                StudioControls.ButtonGroup { id: mirrorGroup }

                StudioControls.AbstractButton {
                    id: mirrorVertical
                    buttonIcon: StudioTheme.Constants.mirror
                    checkable: true
                    autoExclusive: true
                    StudioControls.ButtonGroup.group: mirrorGroup
                    checked: true
                }

                StudioControls.AbstractButton {
                    id: mirrorHorizontal
                    buttonIcon: StudioTheme.Constants.mirror
                    checkable: true
                    autoExclusive: true
                    StudioControls.ButtonGroup.group: mirrorGroup
                    iconRotation: 90
                }
            }

            ExpandingSpacer {}
        }

        PropertyLabel { text: qsTr("Scale") }

        SecondColumnLayout {
            SpinBox {
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                sliderIndicatorVisible: true
                backendValue: backendValues.scale
                decimals: 2
                stepSize: 0.1
                minimumValue: -10
                maximumValue: 10
                enabled: backendValues.scale.isAvailable
            }

            Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

            ControlLabel { text: "%" }

            ExpandingSpacer {}
        }

        PropertyLabel { text: qsTr("Z Stack") }

        SecondColumnLayout {
            SpinBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.z
                minimumValue: -100
                maximumValue: 100
            }

            ExpandingSpacer {}
        }

        PropertyLabel { text: qsTr("Origin") }

        SecondColumnLayout {
            OriginControl {
                backendValue: backendValues.transformOrigin
                enabled: backendValues.transformOrigin.isAvailable
            }

            ExpandingSpacer {}
        }
    }
}
