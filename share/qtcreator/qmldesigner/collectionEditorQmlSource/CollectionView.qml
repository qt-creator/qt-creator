// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import QtQuickDesignerTheme 1.0
import HelperWidgets 2.0 as HelperWidgets
import StudioTheme 1.0 as StudioTheme
import CollectionEditorBackend

Item {
    id: root
    focus: true

    property var rootView: CollectionEditorBackend.rootView
    property var model: CollectionEditorBackend.model
    property var singleCollectionModel: CollectionEditorBackend.singleCollectionModel

    function showWarning(title, message) {
        warningDialog.title = title
        warningDialog.message = message
        warningDialog.open()
    }

    JsonImport {
        id: jsonImporter

        backendValue: root.rootView
        anchors.centerIn: parent
    }

    CsvImport {
        id: csvImporter

        backendValue: root.rootView
        anchors.centerIn: parent
    }

    NewCollectionDialog {
        id: newCollection

        backendValue: root.rootView
        anchors.centerIn: parent
    }

    Message {
        id: warningDialog

        title: ""
        message: ""
    }

    Rectangle {
        id: collectionsRect

        color: StudioTheme.Values.themeToolbarBackground
        width: 300
        height: root.height

        Column {
            width: parent.width

            Rectangle {
                width: parent.width
                height: StudioTheme.Values.height + 5
                color: StudioTheme.Values.themeToolbarBackground

                Text {
                    id: collectionText

                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("Collections")
                    font.pixelSize: StudioTheme.Values.mediumIconFont
                    color: StudioTheme.Values.themeTextColor
                    leftPadding: 15
                }

                Row {
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    rightPadding: 12
                    spacing: 2

                    HelperWidgets.IconButton {
                        icon: StudioTheme.Constants.downloadjson_large
                        tooltip: qsTr("Import Json")

                        onClicked: jsonImporter.open()
                    }

                    HelperWidgets.IconButton {
                        icon: StudioTheme.Constants.downloadcsv_large
                        tooltip: qsTr("Import CSV")

                        onClicked: csvImporter.open()
                    }
                }
            }

            Rectangle { // Collections
                width: parent.width
                color: StudioTheme.Values.themeBackgroundColorNormal
                height: 330

                MouseArea {
                    anchors.fill: parent
                    propagateComposedEvents: true
                    onClicked: (event) => {
                        root.model.deselect()
                        event.accepted = true
                    }
                }

                ListView {
                    id: sourceListView

                    width: parent.width
                    height: contentHeight
                    model: root.model

                    delegate: ModelSourceItem {
                        onDeleteItem: root.model.removeRow(index)
                    }

                }
            }

            Rectangle {
                width: parent.width
                height: addCollectionButton.height
                color: StudioTheme.Values.themeBackgroundColorNormal

                IconTextButton {
                    id: addCollectionButton

                    anchors.centerIn: parent
                    text: qsTr("Add new collection")
                    icon: StudioTheme.Constants.create_medium
                    onClicked: newCollection.open()
                }
            }
        }
    }

    SingleCollectionView {
        model: root.singleCollectionModel
        anchors {
            left: collectionsRect.right
            right: parent.right
            top: parent.top
            bottom: parent.bottom
        }
    }
}
