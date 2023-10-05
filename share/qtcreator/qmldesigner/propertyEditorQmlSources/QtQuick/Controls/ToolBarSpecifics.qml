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
        caption: qsTr("Tool Bar")

        SectionLayout {
            PropertyLabel {
                text: qsTr("Position")
                tooltip: qsTr("Position of the toolbar.")
            }

            SecondColumnLayout {
                ComboBox {
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    width: implicitWidth
                    backendValue: backendValues.position
                    model: [ "Header", "Footer" ]
                    scope: "ToolBar"
                }

                ExpandingSpacer {}
            }
        }
    }

    PaneSection {}

    ControlSection {}

    PaddingSection {}

    InsetSection {}

    FontSection {
        caption: qsTr("Font")
        expanded: false
    }
}
