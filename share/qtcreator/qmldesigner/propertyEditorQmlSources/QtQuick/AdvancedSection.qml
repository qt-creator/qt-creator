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
import StudioTheme 1.0 as StudioTheme

Section {
    anchors.left: parent.left
    anchors.right: parent.right
    caption: qsTr("Advanced")

    SectionLayout {
        PropertyLabel {
            visible: majorQtQuickVersion > 1
            text: qsTr("Enabled")
        }

        SecondColumnLayout {
            visible: majorQtQuickVersion > 1

            CheckBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.enabled
                text: backendValues.enabled.valueToString
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Smooth")
            blockedByTemplate: !backendValues.smooth.isAvailable
        }

        SecondColumnLayout {
            CheckBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.smooth
                text: backendValues.smooth.valueToString
                enabled: backendValues.smooth.isAvailable
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Antialiasing")
            blockedByTemplate: !backendValues.antialiasing.isAvailable
        }

        SecondColumnLayout {
            CheckBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.antialiasing
                text: backendValues.antialiasing.valueToString
                enabled: backendValues.antialiasing.isAvailable
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Focus")
            tooltip: qsTr("Sets focus on the component within the enclosing focus scope.")
            blockedByTemplate: !backendValues.focus.isAvailable
        }

        SecondColumnLayout {
            CheckBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.focus
                text: backendValues.focus.valueToString
                enabled: backendValues.focus.isAvailable
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Focus on tab")
            tooltip: qsTr("Adds the component to the tab focus chain.")
            blockedByTemplate: !backendValues.activeFocusOnTab.isAvailable
        }

        SecondColumnLayout {
            CheckBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.activeFocusOnTab
                text: backendValues.activeFocusOnTab.valueToString
                enabled: backendValues.activeFocusOnTab.isAvailable
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Baseline offset")
            tooltip: qsTr("Position of the component's baseline in local coordinates.")
            blockedByTemplate: !backendValues.baselineOffset.isAvailable
        }

        SecondColumnLayout {
            SpinBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                sliderIndicatorVisible: true
                backendValue: backendValues.baselineOffset
                hasSlider: true
                decimals: 0
                minimumValue: -1000
                maximumValue: 1000
                enabled: backendValues.baselineOffset.isAvailable
            }

            ExpandingSpacer {}
        }
    }
}
