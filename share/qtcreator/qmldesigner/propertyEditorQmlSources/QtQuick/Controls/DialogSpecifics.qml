// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import HelperWidgets 2.0
import QtQuick.Layouts 1.15
import StudioTheme 1.0 as StudioTheme

Column {
    anchors.left: parent.left
    anchors.right: parent.right

    PopupSection {
        anchors.left: parent.left
        anchors.right: parent.right
    }

    Section {
        anchors.left: parent.left
        anchors.right: parent.right
        caption: qsTr("Dialog")

        SectionLayout {
            PropertyLabel { text: qsTr("Title") }

            SecondColumnLayout {
                LineEdit {
                    backendValue: backendValues.title
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    width: implicitWidth
                }

                ExpandingSpacer {}
            }
        }
    }

    MarginSection {
        anchors.left: parent.left
        anchors.right: parent.right
    }

    PaddingSection {
        anchors.left: parent.left
        anchors.right: parent.right
    }

    FontSection {}
}
