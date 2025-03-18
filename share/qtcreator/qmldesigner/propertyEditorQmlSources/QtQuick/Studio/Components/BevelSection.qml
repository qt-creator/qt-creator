// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import HelperWidgets
import StudioTheme as StudioTheme

Section {
    anchors.left: parent.left
    anchors.right: parent.right
    caption: qsTr("Corner Bevel")

    SectionLayout {
        PropertyLabel {
            text: qsTr("Top")
            tooltip: qsTr("Sets a flat cut on the top left or right corners.")
        }

        SecondColumnLayout {
            CheckBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.topLeftBevel
            }

            Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

            MultiIconLabel {
                icon0: StudioTheme.Constants.cornerA
                icon1: StudioTheme.Constants.bevelCorner
            }

            Spacer { implicitWidth: StudioTheme.Values.controlGap }

            CheckBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.topRightBevel
            }

            Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

            MultiIconLabel {
                icon0: StudioTheme.Constants.cornerA
                icon1: StudioTheme.Constants.bevelCorner
                rotation: 90
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Bottom")
            tooltip: qsTr("Sets a flat cut on the bottom left or right corners.")
        }

        SecondColumnLayout {
            CheckBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.bottomLeftBevel
            }

            Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

            MultiIconLabel {
                icon0: StudioTheme.Constants.cornerA
                icon1: StudioTheme.Constants.bevelCorner
                rotation: 270
            }

            Spacer { implicitWidth: StudioTheme.Values.controlGap }

            CheckBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.bottomRightBevel
            }

            Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

            MultiIconLabel {
                icon0: StudioTheme.Constants.cornerA
                icon1: StudioTheme.Constants.bevelCorner
                rotation: 180
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Global")
            tooltip: qsTr("Sets a flat cut on all of the corners.")
        }

        SecondColumnLayout {
            CheckBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.bevel
            }

            Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

            IconLabel { icon: StudioTheme.Constants.bevelAll }

            ExpandingSpacer {}
        }
    }
}
