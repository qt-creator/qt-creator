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
    caption: qsTr("Padding")

    anchors.left: parent.left
    anchors.right: parent.right

    SectionLayout {
        PropertyLabel {
            text: qsTr("Vertical")
            blockedByTemplate: !backendValues.topPadding.isAvailable
                               && !backendValues.bottomPadding.isAvailable
        }

        SecondColumnLayout {
            SpinBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                minimumValue: -10000
                maximumValue: 10000
                decimals: 0
                backendValue: backendValues.topPadding
                enabled: backendValue.isAvailable
            }

            Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

            MultiIconLabel {
                icon0: StudioTheme.Constants.paddingFrame
                icon1: StudioTheme.Constants.paddingEdge
                tooltip: qsTr("Padding between the content and the top edge of the item.")
                enabled: backendValues.topPadding.isAvailable
            }

            Spacer { implicitWidth: StudioTheme.Values.controlGap }

            SpinBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                minimumValue: -10000
                maximumValue: 10000
                decimals: 0
                backendValue: backendValues.bottomPadding
                enabled: backendValue.isAvailable
            }

            Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

            MultiIconLabel {
                icon0: StudioTheme.Constants.paddingFrame
                icon1: StudioTheme.Constants.paddingEdge
                rotation: 180
                tooltip: qsTr("Padding between the content and the bottom edge of the item.")
                enabled: backendValues.bottomPadding.isAvailable
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Horizontal")
            blockedByTemplate: !backendValues.leftPadding.isAvailable
                               && !backendValues.rightPadding.isAvailable
        }

        SecondColumnLayout {
            SpinBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                minimumValue: -10000
                maximumValue: 10000
                decimals: 0
                backendValue: backendValues.leftPadding
                enabled: backendValue.isAvailable
            }

            Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

            MultiIconLabel {
                icon0: StudioTheme.Constants.paddingFrame
                icon1: StudioTheme.Constants.paddingEdge
                rotation: 270
                tooltip: qsTr("Padding between the content and the left edge of the item.")
                enabled: backendValues.leftPadding.isAvailable
            }

            Spacer { implicitWidth: StudioTheme.Values.controlGap }

            SpinBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                minimumValue: -10000
                maximumValue: 10000
                decimals: 0
                backendValue: backendValues.rightPadding
                enabled: backendValue.isAvailable
            }

            Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

            MultiIconLabel {
                icon0: StudioTheme.Constants.paddingFrame
                icon1: StudioTheme.Constants.paddingEdge
                rotation: 90
                tooltip: qsTr("Padding between the content and the right edge of the item.")
                enabled: backendValues.rightPadding.isAvailable
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Global")
            tooltip: qsTr("Padding between the content and the edges of the items.")
            blockedByTemplate: !backendValues.padding.isAvailable
        }

        SecondColumnLayout {
            SpinBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                minimumValue: -10000
                maximumValue: 10000
                decimals: 0
                backendValue: backendValues.padding
                enabled: backendValue.isAvailable
            }

            ExpandingSpacer {}
        }
    }
}
