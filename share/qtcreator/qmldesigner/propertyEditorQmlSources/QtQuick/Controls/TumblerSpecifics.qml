// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import HelperWidgets 2.0
import QtQuick.Layouts 1.15
import StudioTheme 1.0 as StudioTheme

Column {
    width: parent.width

    Section {
        width: parent.width
        caption: qsTr("Tumbler")

        SectionLayout {
            PropertyLabel {
                text: qsTr("Visible count")
                tooltip: qsTr("The count of visible items.")
            }

            SecondColumnLayout {
                SpinBox {
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    maximumValue: 9999999
                    minimumValue: -9999999
                    decimals: 0
                    backendValue: backendValues.visibleItemCount
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Current index")
                tooltip: qsTr("The index of the current item.")
            }

            SecondColumnLayout {
                SpinBox {
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    maximumValue: 9999999
                    minimumValue: -9999999
                    decimals: 0
                    backendValue: backendValues.currentIndex
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Wrap")
                tooltip: qsTr("Whether the tumbler values wrap.")
            }

            SecondColumnLayout {
                CheckBox {
                    text: backendValues.wrap.valueToString
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    backendValue: backendValues.wrap
                }

                ExpandingSpacer {}
            }
        }
    }

    ControlSection {}

    FontSection {}

    PaddingSection {}

    InsetSection {}
}
