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
        caption: qsTr("Loader")
        anchors.left: parent.left
        anchors.right: parent.right

        SectionLayout {
            PropertyLabel {
                text: qsTr("Active")
                tooltip: qsTr("Whether the loader is currently active.")
            }

            SecondColumnLayout {
                CheckBox {
                    text: backendValues.active.valueToString
                    backendValue: backendValues.active
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Source")
                tooltip: qsTr("URL of the component to instantiate.")
            }

            SecondColumnLayout {
                UrlChooser {
                    filter: "*.qml"
                    backendValue: backendValues.source
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Source component")
                tooltip: qsTr("Component to instantiate.")
            }

            SecondColumnLayout {
                ItemFilterComboBox {
                    typeFilter: "Component"
                    backendValue: backendValues.sourceComponent
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Asynchronous")
                tooltip: qsTr("Whether the component will be instantiated asynchronously.")
            }

            SecondColumnLayout {
                CheckBox {
                    text: backendValues.asynchronous.valueToString
                    backendValue: backendValues.asynchronous
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                }

                ExpandingSpacer {}
            }
        }
    }
}
