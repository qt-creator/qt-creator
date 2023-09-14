// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import HelperWidgets as HelperWidgets
import StudioControls as StudioControls
import StudioTheme as StudioTheme
import ConnectionsEditorEditorBackend

Rectangle {
    id: root

    color: StudioTheme.Values.themePanelBackground

    property bool adsFocus: false
    // objectName is used by the dock widget to find this particular ScrollView
    // and set the ads focus on it.
    objectName: "__mainSrollView"

    Column {
        id: column
        anchors.fill: parent
        spacing: 8 // TODO

        Rectangle {
            id: toolbar
            width: parent.width
            height: StudioTheme.Values.doubleToolbarHeight
            color: StudioTheme.Values.themeToolbarBackground

            Column {
                anchors.fill: parent
                anchors.topMargin: StudioTheme.Values.toolbarVerticalMargin
                anchors.bottomMargin: StudioTheme.Values.toolbarVerticalMargin
                anchors.leftMargin: StudioTheme.Values.toolbarHorizontalMargin
                anchors.rightMargin: StudioTheme.Values.toolbarHorizontalMargin
                spacing: StudioTheme.Values.toolbarColumnSpacing

                StudioControls.SearchBox {
                    id: searchBox
                    width: parent.width
                    style: StudioTheme.Values.searchControlStyle

                    onSearchChanged: function(searchText) {}
                }

                Row {
                    id: row
                    width: parent.width
                    height: StudioTheme.Values.toolbarHeight
                    spacing: 6

                    TabCheckButton {
                        id: connections
                        buttonIcon: StudioTheme.Constants.connections_medium
                        text: qsTr("Connections")
                        tooltip: qsTr("This is a tooltip.")
                        checked: true
                        autoExclusive: true
                        checkable: true
                    }

                    TabCheckButton {
                        id: bindings
                        buttonIcon: StudioTheme.Constants.binding_medium
                        text: qsTr("Bindings")
                        tooltip: qsTr("This is a tooltip.")
                        autoExclusive: true
                        checkable: true
                    }

                    TabCheckButton {
                        id: properties
                        buttonIcon: StudioTheme.Constants.properties_medium
                        text: qsTr("Properties")
                        tooltip: qsTr("This is a tooltip.")
                        autoExclusive: true
                        checkable: true
                    }

                    Item {
                        id: spacer
                        width: row.width - connections.width - bindings.width
                               - properties.width - addButton.width - row.spacing * 4
                        height: 1
                    }

                    HelperWidgets.AbstractButton {
                        id: addButton
                        style: StudioTheme.Values.viewBarButtonStyle
                        buttonIcon: StudioTheme.Constants.add_medium
                        tooltip: qsTr("Add something.")
                        onClicked: {
                            if (connections.checked)
                                ConnectionsEditorEditorBackend.connectionModel.add()
                            else if (bindings.checked)
                                ConnectionsEditorEditorBackend.bindingModel.add()
                            else if (properties.checked)
                                ConnectionsEditorEditorBackend.dynamicPropertiesModel.add()
                        }
                    }
                }
            }
        }

        ConnectionsListView {
            visible: connections.checked
            width: parent.width
            height: parent.height - toolbar.height - column.spacing
            model: ConnectionsEditorEditorBackend.connectionModel
            adsFocus: root.adsFocus
        }

        BindingsListView {
            visible: bindings.checked
            width: parent.width
            height: parent.height - toolbar.height - column.spacing
            model: ConnectionsEditorEditorBackend.bindingModel
            adsFocus: root.adsFocus
        }

        PropertiesListView {
            visible: properties.checked
            width: parent.width
            height: parent.height - toolbar.height - column.spacing
            model: ConnectionsEditorEditorBackend.dynamicPropertiesModel
            adsFocus: root.adsFocus
        }
    }
}
