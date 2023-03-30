// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import QtQuick.Layouts 1.15
import HelperWidgets 2.0
import StudioTheme 1.0 as StudioTheme

Column {
    width: parent.width

    Section {
        width: parent.width
        caption: qsTr("Round Button")

        SectionLayout {
            PropertyLabel {
                text: qsTr("Appearance")
                tooltip: qsTr("Toggles if the button is flat or highlighted.")
                blockedByTemplate: !backendValues.flat.isAvailable
                                   && !backendValues.highlighted.isAvailable
            }

            SecondColumnLayout {
                CheckBox {
                    text: qsTr("Flat")
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    backendValue: backendValues.flat
                    enabled: backendValue.isAvailable
                }

                Spacer { implicitWidth: StudioTheme.Values.twoControlColumnGap }

                CheckBox {
                    text: qsTr("Highlight")
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    backendValue: backendValues.highlighted
                    enabled: backendValue.isAvailable
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Radius")
                tooltip: qsTr("Sets the radius of the button.")
            }

            SecondColumnLayout {
                SpinBox {
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    minimumValue: 0
                    maximumValue: 10000
                    decimals: 0
                    backendValue: backendValues.radius
                }

                ExpandingSpacer {}
            }
        }
    }

    AbstractButtonSection {}

    IconSection {}

    ControlSection {}

    FontSection {}

    PaddingSection {}

    InsetSection {}
}
