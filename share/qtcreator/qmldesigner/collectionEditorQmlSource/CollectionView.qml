// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuickDesignerTheme 1.0
import HelperWidgets 2.0 as HelperWidgets
import StudioTheme 1.0 as StudioTheme
import CollectionEditorBackend

Item {
    id: root
    focus: true

    property var rootView: CollectionEditorBackend.rootView
    property var model: CollectionEditorBackend.model
    property var collectionDetailsModel: CollectionEditorBackend.collectionDetailsModel
    property var collectionDetailsSortFilterModel: CollectionEditorBackend.collectionDetailsSortFilterModel

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
        sourceModel: root.model
        anchors.centerIn: parent
    }

    Message {
        id: warningDialog

        title: ""
        message: ""
    }

    GridLayout {
        id: grid
        readonly property bool isHorizontal: width >= 500

        anchors.fill: parent
        columns: isHorizontal ? 3 : 1

        ColumnLayout {
            id: collectionsSideBar

            Layout.alignment: Qt.AlignTop | Qt.AlignLeft
            Layout.minimumWidth: 300
            Layout.fillWidth: !grid.isHorizontal

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: StudioTheme.Values.height + 5
                color: StudioTheme.Values.themeToolbarBackground

                Text {
                    id: collectionText

                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("Data Models")
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
                        tooltip: qsTr("Import JSON")

                        onClicked: jsonImporter.open()
                    }

                    HelperWidgets.IconButton {
                        icon: StudioTheme.Constants.downloadcsv_large
                        tooltip: qsTr("Import CSV")

                        onClicked: csvImporter.open()
                    }
                }
            }

            Rectangle { // Model Groups
                Layout.fillWidth: true
                color: StudioTheme.Values.themeBackgroundColorNormal
                Layout.minimumHeight: 150
                Layout.preferredHeight: sourceListView.contentHeight

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

                    anchors.fill: parent
                    model: root.model

                    delegate: ModelSourceItem {
                        implicitWidth: sourceListView.width
                        onDeleteItem: root.model.removeRow(index)
                        hasSelectedTarget: root.rootView.targetNodeSelected
                        onAssignToSelected: root.rootView.assignSourceNodeToSelectedItem(sourceNode)
                    }
                }
            }

            HelperWidgets.IconButton {
                id: addCollectionButton

                iconSize:16
                Layout.fillWidth: true
                Layout.minimumWidth: 24
                Layout.alignment: Qt.AlignTop | Qt.AlignHCenter

                tooltip: qsTr("Add a new model")
                icon: StudioTheme.Constants.create_medium
                onClicked: newCollection.open()
            }
        }

        Rectangle { // Splitter
            Layout.fillWidth: !grid.isHorizontal
            Layout.fillHeight: grid.isHorizontal
            Layout.minimumWidth: 2
            Layout.minimumHeight: 2
            color: "black"
        }

        CollectionDetailsView {
            model: root.collectionDetailsModel
            backend: root.model
            sortedModel: root.collectionDetailsSortFilterModel
            Layout.fillHeight: true
            Layout.fillWidth: true
        }
    }
}
