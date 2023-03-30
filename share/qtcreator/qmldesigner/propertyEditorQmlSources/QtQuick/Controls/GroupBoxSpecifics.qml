// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import QtQuick.Layouts 1.15
import HelperWidgets 2.0
import StudioTheme 1.0 as StudioTheme

Column {
    width: parent.width

    Section {
        caption: qsTr("Group Box")

        width: parent.width

        SectionLayout {
            PropertyLabel {
                text: qsTr("Title")
                tooltip: qsTr("Sets the title for the group box.")
            }

            SecondColumnLayout {
                LineEdit {
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    width: implicitWidth
                    backendValue: backendValues.title
                }

                ExpandingSpacer {}
            }
        }
    }

    PaneSection {}

    ControlSection {}

    FontSection {}

    PaddingSection {}

    InsetSection {}
}
