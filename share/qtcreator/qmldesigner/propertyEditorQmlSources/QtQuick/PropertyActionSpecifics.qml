// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import HelperWidgets 2.0
import QtQuick 2.15
import QtQuick.Layouts 1.15
import StudioTheme 1.0 as StudioTheme

Column {
    anchors.left: parent.left
    anchors.right: parent.right

    Section {
        id: section
        caption: qsTr("Property Action")

        anchors.left: parent.left
        anchors.right: parent.right

        SectionLayout {
            PropertyLabel {
                text: qsTr("Value")
                tooltip: qsTr("Value of the property.")
            }

            SecondColumnLayout {
                LineEdit {
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                + StudioTheme.Values.actionIndicatorWidth
                    width: implicitWidth
                    backendValue: backendValues.value
                }

                ExpandingSpacer {}
            }
        }
    }

    AnimationTargetSection {}

    AnimationSection { showDuration: false }
}

