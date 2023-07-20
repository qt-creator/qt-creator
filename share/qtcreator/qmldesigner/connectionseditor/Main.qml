// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ConnectionsEditor
import HelperWidgets 2.0 as HelperWidgets
import StudioControls 1.0 as StudioControls
import StudioTheme as StudioTheme
import ConnectionsEditorEditorBackend

Rectangle {
    width: 640
    height: 1080
    color: "#232222"

    Rectangle {
        id: rectangle
        x: 10
        y: 10
        width: 620
        height: 97
        color: "#333333"

        Rectangle {
            id: rectangle1
            x: 10
            y: 10
            width: 600
            height: 34
            color: "#00ffffff"
            border.width: 1

            Text {
                id: text1
                x: 10
                y: 10
                color: "#b5b2b2"
                text: qsTr("Search")
                font.pixelSize: 12
            }
        }

        RowLayout {
            x: 10
            y: 50
            TabCheckButton {
                id: connections
                text: "Connections"
                checked: true
                autoExclusive: true
                checkable: true
            }

            TabCheckButton {
                id: bindings
                text: "Bindings"
                autoExclusive: true
                checkable: true
            }

            TabCheckButton {
                id: properties
                text: "Properties"
                autoExclusive: true
                checkable: true
            }
        }

        Text {
            id: text2
            x: 577
            y: 58
            color: "#ffffff"
            text: qsTr("+")
            font.pixelSize: 18
            font.bold: true
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    print(ConnectionsEditorEditorBackend.dynamicPropertiesModel.delegate)
                    print(ConnectionsEditorEditorBackend.dynamicPropertiesModel.delegate.type)
                    print(ConnectionsEditorEditorBackend.dynamicPropertiesModel.delegate.type.model)

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

    ConnectionsListView {
        visible: connections.checked
        x: 17
        y: 124
        model: ConnectionsEditorEditorBackend.connectionModel
    }

    BindingsListView {
        visible: bindings.checked
        x: 17
        y: 124
        model: ConnectionsEditorEditorBackend.bindingModel
    }

    PropertiesListView {
        visible: properties.checked
        x: 17
        y: 124
        model: ConnectionsEditorEditorBackend.dynamicPropertiesModel
    }
}
