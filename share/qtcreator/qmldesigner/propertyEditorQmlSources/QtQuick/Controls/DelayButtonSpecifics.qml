// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import QtQuick.Layouts 1.15
import HelperWidgets 2.0
import StudioTheme 1.0 as StudioTheme

Column {
    width: parent.width

    Section {
        caption: qsTr("Delay Button")

        width: parent.width

        SectionLayout {
            PropertyLabel {
                text: qsTr("Delay")
                tooltip: qsTr("Sets the delay before the button activates.")
            }

            SecondColumnLayout {
                SpinBox {
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                    width: implicitWidth
                    minimumValue: 0
                    maximumValue: 9999999
                    decimals: 0
                    stepSize: 1
                    backendValue: backendValues.delay
                }

                Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

                ControlLabel {
                    text: "ms"
                    tooltip: qsTr("Milliseconds.")
                    elide: Text.ElideNone
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
