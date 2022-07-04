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

import QtQuick 2.15
import QtQuick.Layouts 1.15
import HelperWidgets 2.0
import StudioTheme 1.0 as StudioTheme

Section {
    caption: qsTr("Inset")

    width: parent.width

    SectionLayout {
        PropertyLabel { text: qsTr("Vertical") }

        SecondColumnLayout {
            SpinBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                maximumValue: 10000
                minimumValue: -10000
                realDragRange: 5000
                decimals: 0
                backendValue: backendValues.topInset
            }

            Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

            MultiIconLabel {
                icon0: StudioTheme.Constants.paddingFrame
                icon1: StudioTheme.Constants.paddingEdge
                tooltip: qsTr("Top inset for the background.")
                enabled: !backendValues.topPadding.isAvailable
            }

            Spacer { implicitWidth: StudioTheme.Values.controlGap }

            SpinBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                maximumValue: 10000
                minimumValue: -10000
                realDragRange: 5000
                decimals: 0
                backendValue: backendValues.bottomInset
            }

            Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

            MultiIconLabel {
                icon0: StudioTheme.Constants.paddingFrame
                icon1: StudioTheme.Constants.paddingEdge
                rotation: 180
                tooltip: qsTr("Bottom inset for the background.")
                enabled: !backendValues.bottomPadding.isAvailable
            }

            ExpandingSpacer {}
        }

        PropertyLabel { text: qsTr("Horizontal") }

        SecondColumnLayout {
            SpinBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                maximumValue: 10000
                minimumValue: -10000
                realDragRange: 5000
                decimals: 0
                backendValue: backendValues.leftInset
            }

            Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

            MultiIconLabel {
                icon0: StudioTheme.Constants.paddingFrame
                icon1: StudioTheme.Constants.paddingEdge
                rotation: 270
                tooltip: qsTr("Left inset for the background.")
                enabled: !backendValues.leftPadding.isAvailable
            }

            Spacer { implicitWidth: StudioTheme.Values.controlGap }

            SpinBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                maximumValue: 10000
                minimumValue: -10000
                realDragRange: 5000
                decimals: 0
                backendValue: backendValues.rightInset
            }

            Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

            MultiIconLabel {
                icon0: StudioTheme.Constants.paddingFrame
                icon1: StudioTheme.Constants.paddingEdge
                rotation: 90
                tooltip: qsTr("Right inset for the background.")
                enabled: !backendValues.rightPadding.isAvailable
            }

            ExpandingSpacer {}
        }
    }
}
