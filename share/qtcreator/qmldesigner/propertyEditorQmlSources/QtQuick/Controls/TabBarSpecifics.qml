// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import QtQuick.Layouts 1.15
import HelperWidgets 2.0
import StudioControls 1.0 as StudioControls
import StudioTheme 1.0 as StudioTheme

Column {
    width: parent.width

    Section {
        width: parent.width
        caption: qsTr("Tab Bar")

        SectionLayout {
            PropertyLabel {
                text: qsTr("Position")
                tooltip: qsTr("Sets the position of the tab bar.")
            }

            SecondColumnLayout {
                ComboBox {
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    width: implicitWidth
                    backendValue: backendValues.position
                    model: [ "Header", "Footer" ]
                    scope: "TabBar"
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Content size")
                tooltip: qsTr("Sets the width and height of the tab bar.\n"
                        + "This is used for calculating the total implicit size.")
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

    ContainerSection {}

    ControlSection {}

    FontSection {}

    PaddingSection {}

    InsetSection {}
}
