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
    id: root
    caption: qsTr("Flickable Geometry")

    anchors.left: parent.left
    anchors.right: parent.right

    SectionLayout {
        PropertyLabel { text: qsTr("Content size") }

        SecondColumnLayout {
            SpinBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.contentWidth
                minimumValue: 0
                maximumValue: 10000
            }

            Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

            ControlLabel {
                //: The width of the object
                text: qsTr("W", "width")
                tooltip: qsTr("Content width used for calculating the total implicit width.")
            }

            Spacer { implicitWidth: StudioTheme.Values.controlGap }

            SpinBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.contentHeight
                minimumValue: 0
                maximumValue: 10000
            }

            Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

            ControlLabel {
                //: The height of the object
                text: qsTr("H", "height")
                tooltip: qsTr("Content height used for calculating the total implicit height.")
            }
/*
            TODO QDS-4836
            Spacer { implicitWidth: StudioTheme.Values.controlGap }

            LinkIndicator2D {}
*/
            ExpandingSpacer {}
        }

        PropertyLabel { text: qsTr("Content") }

        SecondColumnLayout {
            SpinBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.contentX
                minimumValue: -8000
                maximumValue: 8000
            }

            Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

            ControlLabel { text: "X" }

            Spacer { implicitWidth: StudioTheme.Values.controlGap }

            SpinBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.contentY
                minimumValue: -8000
                maximumValue: 8000
            }

            Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

            ControlLabel { text: "Y" }
/*
            TODO QDS-4836
            Spacer { implicitWidth: StudioTheme.Values.controlGap }

            LinkIndicator2D {}
*/
            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Origin")
            blockedByTemplate: !backendValues.originX.isAvailable
                               && !backendValues.originY.isAvailable
        }

        SecondColumnLayout {
            SpinBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.originX
                minimumValue: -8000
                maximumValue: 8000
                enabled: backendValue.isAvailable
            }

            Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

            ControlLabel {
                text: "X"
                enabled: backendValues.originX.isAvailable
            }

            Spacer { implicitWidth: StudioTheme.Values.controlGap }

            SpinBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.originY
                minimumValue: -8000
                maximumValue: 8000
                enabled: backendValue.isAvailable
            }

            Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

            ControlLabel {
                text: "Y"
                enabled: backendValues.originY.isAvailable
            }
/*
            TODO QDS-4836
            Spacer { implicitWidth: StudioTheme.Values.controlGap }

            LinkIndicator2D {}
*/
            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Left margin")
            blockedByTemplate: !backendValues.leftMargin.isAvailable
        }

        SecondColumnLayout {
            SpinBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.leftMargin
                minimumValue: -10000
                maximumValue: 10000
                decimals: 0
                enabled: backendValue.isAvailable
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Right margin")
            blockedByTemplate: !backendValues.rightMargin.isAvailable
        }

        SecondColumnLayout {
            SpinBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.rightMargin
                minimumValue: -10000
                maximumValue: 10000
                decimals: 0
                enabled: backendValue.isAvailable
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Top margin")
            blockedByTemplate: !backendValues.topMargin.isAvailable
        }

        SecondColumnLayout {
            SpinBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.topMargin
                minimumValue: -10000
                maximumValue: 10000
                decimals: 0
                enabled: backendValue.isAvailable
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Bottom margin")
            blockedByTemplate: !backendValues.bottomMargin.isAvailable
        }

        SecondColumnLayout {
            SpinBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.bottomMargin
                minimumValue: -10000
                maximumValue: 10000
                decimals: 0
                enabled: backendValue.isAvailable
            }

            ExpandingSpacer {}
        }
    }
}
