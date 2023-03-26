// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import QtQuick.Layouts 1.15
import HelperWidgets 2.0
import StudioTheme 1.0 as StudioTheme

Column {
    anchors.left: parent.left
    anchors.right: parent.right

    Section {
        anchors.left: parent.left
        anchors.right: parent.right
        caption: qsTr("State")

        SectionLayout {
            PropertyLabel {
                text: qsTr("When")
                tooltip: qsTr("Sets when the state should be applied.")
            }

            SecondColumnLayout {
                CheckBox {
                    text: backendValues.when.valueToString
                    backendValue: backendValues.when
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Name")
                tooltip: qsTr("The name of the state.")
            }

            SecondColumnLayout {
                LineEdit {
                    backendValue: backendValues.name
                    showTranslateCheckBox: false
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    width: implicitWidth
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Extend")
                tooltip: qsTr("The state that this state extends.")
            }

            SecondColumnLayout {
                LineEdit {
                    backendValue: backendValues.extend
                    showTranslateCheckBox: false
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    width: implicitWidth
                }

                ExpandingSpacer {}
            }
        }

    }
}
