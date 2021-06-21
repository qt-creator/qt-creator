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
    caption: qsTr("Margin")

    anchors.left: parent.left
    anchors.right: parent.right

    property alias backendValueTopMargin: spinBoxTopMargin.backendValue
    property alias backendValueBottomMargin: spinBoxBottomMargin.backendValue
    property alias backendValueLeftMargin: spinBoxLeftMargin.backendValue
    property alias backendValueRightMargin: spinBoxRightMargin.backendValue
    property alias backendValueMargins: spinBoxMargins.backendValue

    SectionLayout {
        PropertyLabel { text: qsTr("Vertical") }

        SecondColumnLayout {
            SpinBox {
                id: spinBoxTopMargin
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                minimumValue: -10000
                maximumValue: 10000
                decimals: 0
                backendValue: backendValues.topMargin
            }

            Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

            MultiIconLabel {
                icon0: StudioTheme.Constants.paddingFrame
                icon1: StudioTheme.Constants.paddingEdge
                iconColor: StudioTheme.Values.themeTextColor
                tooltip: qsTr("The margin above the item.")
            }

            Spacer { implicitWidth: StudioTheme.Values.controlGap }

            SpinBox {
                id: spinBoxBottomMargin
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                minimumValue: -10000
                maximumValue: 10000
                decimals: 0
                backendValue: backendValues.bottomMargin
            }

            Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

            MultiIconLabel {
                icon0: StudioTheme.Constants.paddingFrame
                icon1: StudioTheme.Constants.paddingEdge
                iconColor: StudioTheme.Values.themeTextColor
                rotation: 180
                tooltip: qsTr("The margin below the item.")
            }

            ExpandingSpacer {}
        }

        PropertyLabel { text: qsTr("Horizontal") }

        SecondColumnLayout {
            SpinBox {
                id: spinBoxLeftMargin
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                minimumValue: -10000
                maximumValue: 10000
                decimals: 0
                backendValue: backendValues.leftMargin
            }

            Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

            MultiIconLabel {
                icon0: StudioTheme.Constants.paddingFrame
                icon1: StudioTheme.Constants.paddingEdge
                iconColor: StudioTheme.Values.themeTextColor
                rotation: 270
                tooltip: qsTr("The margin left of the item.")
            }

            Spacer { implicitWidth: StudioTheme.Values.controlGap }

            SpinBox {
                id: spinBoxRightMargin
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                minimumValue: -10000
                maximumValue: 10000
                decimals: 0
                backendValue: backendValues.rightMargin
            }

            Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

            MultiIconLabel {
                icon0: StudioTheme.Constants.paddingFrame
                icon1: StudioTheme.Constants.paddingEdge
                iconColor: StudioTheme.Values.themeTextColor
                rotation: 90
                tooltip: qsTr("The margin right of the item.")
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Margins")
            tooltip: qsTr("The margins around the item.")
        }

        SecondColumnLayout {
            SpinBox {
                id: spinBoxMargins
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                minimumValue: -10000
                maximumValue: 10000
                decimals: 0
                backendValue: backendValues.margins
            }

            ExpandingSpacer {}
        }
    }
}
