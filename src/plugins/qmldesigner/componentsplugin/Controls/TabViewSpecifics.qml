// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.1
import HelperWidgets 2.0
import QtQuick.Layouts 1.0
import QtQuick.Controls 1.1 as Controls

Column {
    anchors.left: parent.left
    anchors.right: parent.right

    Section {
        anchors.left: parent.left
        anchors.right: parent.right
        caption: qsTr("Tab View")

        SectionLayout {

            Label {
                text: qsTr("Current index")
            }

            CurrentIndexComboBox {
            }


            Label {
                text: qsTr("Frame visible")
                tooltip: qsTr("Determines the visibility of the tab frame around contents.")
            }

            SecondColumnLayout {
                CheckBox {
                    text: backendValues.frameVisible.valueToString
                    backendValue: backendValues.frameVisible
                    implicitWidth: 180
                }
                ExpandingSpacer {

                }
            }

            Label {
                text: qsTr("Tabs visible")
                tooltip: qsTr("Determines the visibility of the tab bar.")
            }

            SecondColumnLayout {
                CheckBox {
                    text: backendValues.tabsVisible.valueToString
                    backendValue: backendValues.tabsVisible
                    implicitWidth: 180
                }
                ExpandingSpacer {

                }
            }

            Label {
                text: qsTr("Tab position")
                tooltip: qsTr("Determines the position of the tabs.")
            }

            SecondColumnLayout {

                TabPositionComboBox {
                }

                ExpandingSpacer {

                }
            }



        }
    }
}
