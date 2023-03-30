// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import QtQuick.Layouts 1.15
import HelperWidgets 2.0
import StudioTheme 1.0 as StudioTheme

Section {
    caption: qsTr("Pane")

    width: parent.width

    SectionLayout {
        PropertyLabel {
            text: qsTr("Content size")
            tooltip: qsTr("Sets the size of the %1. This is used to calculate\n"
                        + "the total implicit size.").arg(caption.charAt(0).toLowerCase()
                                                          + caption.slice(1))
        }

        SecondColumnLayout {
            SpinBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                width: implicitWidth
                maximumValue: 9999999
                minimumValue: -9999999
                decimals: 0
                backendValue: backendValues.contentWidth
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
                width: implicitWidth
                maximumValue: 9999999
                minimumValue: -9999999
                decimals: 0
                backendValue: backendValues.contentHeight
            }

            Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

            ControlLabel {
                //: The height of the object
                text: qsTr("H", "height")
                tooltip: qsTr("Content height used for calculating the total implicit height.")
            }

            ExpandingSpacer {}
        }
    }
}
