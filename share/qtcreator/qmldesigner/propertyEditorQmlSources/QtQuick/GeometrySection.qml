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
import QtQuick.Layouts 1.15
import HelperWidgets 2.0
import StudioControls 1.0 as StudioControls
import StudioTheme 1.0 as StudioTheme

Section {
    id: root
    caption: qsTr("Geometry - 2D")

    anchors.left: parent.left
    anchors.right: parent.right

    readonly property string disabledTooltip: qsTr("This property is defined by an anchor or a layout.")

    function positionDisabled() {
        return anchorBackend.isFilled || anchorBackend.isInLayout
    }

    function xDisabled() {
        return anchorBackend.leftAnchored
               || anchorBackend.rightAnchored
               || anchorBackend.horizontalCentered
    }

    function yDisabled() {
        return anchorBackend.topAnchored
               || anchorBackend.bottomAnchored
               || anchorBackend.verticalCentered
    }

    function sizeDisabled() {
        return anchorBackend.isFilled
    }

    function widthDisabled() {
        return anchorBackend.leftAnchored && anchorBackend.rightAnchored
    }

    function heightDisabled() {
        return anchorBackend.topAnchored && anchorBackend.bottomAnchored
    }

    SectionLayout {
        PropertyLabel {
            text: qsTr("Position")
            enabled: xSpinBox.enabled || ySpinBox.enabled
        }

        SecondColumnLayout {
            SpinBox {
                id: xSpinBox
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.x
                maximumValue: 0xffff
                minimumValue: -0xffff
                decimals: 0
                enabled: !root.positionDisabled() && !root.xDisabled()
            }

            Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

            ControlLabel {
                text: "X"
                tooltip: xSpinBox.enabled ? "X" : root.disabledTooltip
                enabled: xSpinBox.enabled
            }

            Spacer { implicitWidth: StudioTheme.Values.controlGap }

            SpinBox {
                id: ySpinBox
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.y
                maximumValue: 0xffff
                minimumValue: -0xffff
                decimals: 0
                enabled: !root.positionDisabled() && !root.yDisabled()
            }

            Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

            ControlLabel {
                text: "Y"
                tooltip: xSpinBox.enabled ? "Y" : root.disabledTooltip
                enabled: ySpinBox.enabled
            }
/*
            TODO QDS-4836
            Spacer { implicitWidth: StudioTheme.Values.controlGap }

            LinkIndicator2D {}
*/
            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Size")
            enabled: widthSpinBox.enabled || heightSpinBox.enabled
        }

        SecondColumnLayout {
            SpinBox {
                id: widthSpinBox
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.width
                maximumValue: 0xffff
                minimumValue: 0
                decimals: 0
                enabled: !root.sizeDisabled() && !root.widthDisabled()
            }

            Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

            ControlLabel {
                //: The width of the object
                text: qsTr("W", "width")
                tooltip: widthSpinBox.enabled ? qsTr("Width") : root.disabledTooltip
                enabled: widthSpinBox.enabled
            }

            Spacer { implicitWidth: StudioTheme.Values.controlGap }

            SpinBox {
                id: heightSpinBox
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.height
                maximumValue: 0xffff
                minimumValue: 0
                decimals: 0
                enabled: !root.sizeDisabled() && !root.heightDisabled()
            }

            Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

            ControlLabel {
                //: The height of the object
                text: qsTr("H", "height")
                tooltip: heightSpinBox.enabled ? qsTr("Height") : root.disabledTooltip
                enabled: heightSpinBox.enabled
            }
/*
            TODO QDS-4836
            Spacer { implicitWidth: StudioTheme.Values.controlGap }

            LinkIndicator2D {}
*/
            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Rotation")
            blockedByTemplate: !backendValues.rotation.isAvailable
        }

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

            ControlLabel {
                text: "°"
                enabled: backendValues.rotation.isAvailable
            }
/*
            TODO QDS-4835
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
*/
            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Scale")
            blockedByTemplate: !backendValues.scale.isAvailable
        }

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

            ControlLabel {
                text: "%"
                enabled: backendValues.scale.isAvailable
            }

            ExpandingSpacer {}
        }

        PropertyLabel { text: qsTr("Z stack") }

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

        PropertyLabel {
            text: qsTr("Origin")
            blockedByTemplate: !backendValues.transformOrigin.isAvailable
        }

        SecondColumnLayout {
            OriginControl {
                backendValue: backendValues.transformOrigin
                enabled: backendValues.transformOrigin.isAvailable
            }

            ExpandingSpacer {}
        }
    }
}
