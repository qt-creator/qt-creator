// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import HelperWidgets as HelperWidgets
import StudioTheme as StudioTheme
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

    ImportDialog {
        id: importDialog

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

        columnSpacing: 0
        rowSpacing: 0

        anchors.fill: parent
        columns: isHorizontal ? 3 : 1

        ColumnLayout {
            id: collectionsSideBar
            spacing: 0

            Layout.alignment: Qt.AlignTop | Qt.AlignLeft
            Layout.minimumWidth: 300
            Layout.fillWidth: !grid.isHorizontal

            Rectangle {
                color: StudioTheme.Values.themeToolbarBackground

                Layout.preferredHeight: StudioTheme.Values.toolbarHeight
                Layout.fillWidth: true

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    anchors.leftMargin: StudioTheme.Values.toolbarHorizontalMargin

                    text: qsTr("Data Models")
                    font.pixelSize: StudioTheme.Values.baseFontSize
                    color: StudioTheme.Values.themeTextColor
                }

                HelperWidgets.AbstractButton {
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.right: parent.right
                    anchors.rightMargin: StudioTheme.Values.toolbarHorizontalMargin

                    style: StudioTheme.Values.viewBarButtonStyle
                    buttonIcon: StudioTheme.Constants.import_medium
                    tooltip: qsTr("Import a model")

                    onClicked: importDialog.open()
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
                        hasSelectedTarget: root.rootView.targetNodeSelected
                    }
                }
            }

            HelperWidgets.IconButton {
                id: addCollectionButton

                iconSize: 16
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
