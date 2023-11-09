// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import HelperWidgets 2.0 as HelperWidgets
import StudioTheme 1.0 as StudioTheme
import StudioControls 1.0 as StudioControls

Rectangle {
    id: root

    required property var model
    required property var backend
    required property var sortedModel

    implicitWidth: 300
    implicitHeight: 400

    color: StudioTheme.Values.themeControlBackground

    ColumnLayout {
        id: topRow

        visible: collectionNameText.text !== ""

        spacing: 0
        anchors {
            fill: parent
            topMargin: 10
            leftMargin: 15
            rightMargin: 15
            bottomMargin: 10
        }

        Text {
            id: collectionNameText

            leftPadding: 8
            rightPadding: 8
            topPadding: 3
            bottomPadding: 3

            color: StudioTheme.Values.themeTextColor
            text: root.model.collectionName
            font.pixelSize: StudioTheme.Values.baseFontSize
            elide: Text.ElideRight
        }

        Item { // spacer
            implicitWidth: 1
            implicitHeight: 10
        }

        CollectionDetailsToolbar {
            id: toolbar
            model: root.model
            backend: root.backend
            Layout.fillWidth: true
            Layout.minimumWidth: implicitWidth
        }

        Item { // spacer
            implicitWidth: 1
            implicitHeight: 5
        }

        GridLayout {
            columns: 3
            rowSpacing: 1
            columnSpacing: 1

            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.maximumWidth: parent.width

            Rectangle {
                clip: true
                visible: !tableView.model.isEmpty
                color: StudioTheme.Values.themeControlBackgroundInteraction
                border.color: StudioTheme.Values.themeControlBackgroundInteraction
                border.width: 2

                Layout.preferredWidth: rowIdView.width
                Layout.preferredHeight: headerView.height
                Layout.minimumWidth: rowIdView.width
                Layout.minimumHeight: headerView.height

                Text {
                    anchors.fill: parent
                    font: headerTextMetrics.font
                    text: "#"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    color: StudioTheme.Values.themeTextColor
                }
            }

            HorizontalHeaderView {
                id: headerView

                property real topPadding: 5
                property real bottomPadding: 5

                Layout.preferredHeight: headerTextMetrics.height + topPadding + bottomPadding
                Layout.columnSpan: 2
                syncView: tableView
                clip: true

                delegate: HeaderDelegate {
                    id: horizontalHeaderItem

                    selectedItem: tableView.model.selectedColumn
                    color: StudioTheme.Values.themeControlBackgroundInteraction

                    function getGlobalBottomLeft() {
                        return mapToGlobal(0, horizontalHeaderItem.height)
                    }

                    MouseArea {
                        anchors.fill: parent
                        anchors.margins: 5
                        acceptedButtons: Qt.LeftButton | Qt.RightButton
                        onClicked: (mouse) => {
                            tableView.model.selectColumn(index)

                            if (mouse.button === Qt.RightButton)
                                headerMenu.popIndex(index, horizontalHeaderItem.getGlobalBottomLeft())
                        }
                    }
                }

                StudioControls.Menu {
                    id: headerMenu

                    property int clickedHeader: -1
                    property point initialPosition

                    function popIndex(clickedIndex, clickedRect)
                    {
                        headerMenu.clickedHeader = clickedIndex
                        headerMenu.initialPosition = clickedRect
                        headerMenu.popup()
                    }

                    onClosed: {
                        headerMenu.clickedHeader = -1
                    }

                    StudioControls.MenuItem {
                        text: qsTr("Edit")
                        onTriggered: editProperyDialog.editProperty(headerMenu.clickedHeader, headerMenu.initialPosition)
                    }

                    StudioControls.MenuItem {
                        text: qsTr("Delete")
                        onTriggered: deleteColumnDialog.popUp(headerMenu.clickedHeader)
                    }

                    StudioControls.MenuItem {
                        text: qsTr("Sort Ascending")
                        onTriggered: sortedModel.sort(headerMenu.clickedHeader, Qt.AscendingOrder)
                    }

                    StudioControls.MenuItem {
                        text: qsTr("Sort Descending")
                        onTriggered: sortedModel.sort(headerMenu.clickedHeader, Qt.DescendingOrder)
                    }
                }
            }

            VerticalHeaderView {
                id: rowIdView

                syncView: tableView
                clip: true

                Layout.preferredHeight: tableView.height
                Layout.rowSpan: 2
                Layout.alignment: Qt.AlignTop + Qt.AlignLeft

                delegate: HeaderDelegate {
                    selectedItem: tableView.model.selectedRow
                    color: StudioTheme.Values.themeControlBackgroundHover

                    MouseArea {
                        anchors.fill: parent
                        anchors.margins: 5
                        acceptedButtons: Qt.LeftButton
                        onClicked: tableView.model.selectRow(index)
                    }
                }
            }

            TableView {
                id: tableView

                model: root.sortedModel
                clip: true

                Layout.preferredWidth: tableView.contentWidth
                Layout.preferredHeight: tableView.contentHeight
                Layout.minimumWidth: 100
                Layout.minimumHeight: 20
                Layout.maximumWidth: root.width

                delegate: Rectangle {
                    id: itemCell
                    implicitWidth: 100
                    implicitHeight: itemText.height
                    border.width: 1

                    Text {
                        id: itemText

                        text: display ? display : ""

                        width: parent.width
                        leftPadding: 5
                        topPadding: 3
                        bottomPadding: 3
                        font.pixelSize: StudioTheme.Values.baseFontSize
                        horizontalAlignment: Text.AlignLeft
                        verticalAlignment: Text.AlignVCenter
                        elide: Text.ElideRight
                    }

                    TableView.editDelegate: CollectionDetailsEditDelegate {
                        anchors {
                            top: itemText.top
                            left: itemText.left
                        }
                    }

                    states: [
                        State {
                            name: "default"
                            when: !itemSelected

                            PropertyChanges {
                                target: itemCell
                                color: StudioTheme.Values.themeControlBackground
                                border.color: StudioTheme.Values.themeControlBackgroundInteraction
                            }

                            PropertyChanges {
                                target: itemText
                                color: StudioTheme.Values.themePlaceholderTextColorInteraction
                            }
                        },
                        State {
                            name: "selected"
                            when: itemSelected

                            PropertyChanges {
                                target: itemCell
                                color: StudioTheme.Values.themeControlBackgroundInteraction
                                border.color: StudioTheme.Values.themeControlBackground
                            }

                            PropertyChanges {
                                target: itemText
                                color: StudioTheme.Values.themeInteraction
                            }
                        }
                    ]
                }
            }

            HelperWidgets.IconButton {
                id: addColumnContainer

                iconSize:16
                Layout.preferredWidth: 24
                Layout.preferredHeight: tableView.height
                Layout.minimumHeight: 24
                Layout.alignment: Qt.AlignLeft + Qt.AlignVCenter

                icon: StudioTheme.Constants.create_medium
                tooltip: "Add Column"

                onClicked: toolbar.addNewColumn()
            }

            HelperWidgets.IconButton {
                id: addRowContainer

                iconSize:16
                Layout.preferredWidth: tableView.width
                Layout.preferredHeight: 24
                Layout.minimumWidth: 24
                Layout.alignment: Qt.AlignTop + Qt.AlignHCenter

                icon: StudioTheme.Constants.create_medium
                tooltip: "Add Row"

                onClicked: toolbar.addNewRow()
            }

            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true
            }
        }
    }

    Text {
        anchors.fill: parent
        text: qsTr("Select a model to continue")
        visible: !topRow.visible
        textFormat: Text.RichText
        color: StudioTheme.Values.themeTextColor
        font.pixelSize: StudioTheme.Values.mediumFontSize
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.WordWrap
    }

    TextMetrics {
        id: headerTextMetrics

        font.pixelSize: StudioTheme.Values.baseFontSize
        text: "Xq"
    }

    component HeaderDelegate: Rectangle {
        id: headerItem

        required property int selectedItem
        property alias horizontalAlignment: headerText.horizontalAlignment
        property alias verticalAlignment: headerText.verticalAlignment

        implicitWidth: headerText.implicitWidth
        implicitHeight: headerText.implicitHeight
        border.width: 1
        clip: true

        Text {
            id: headerText

            topPadding: headerView.topPadding
            bottomPadding: headerView.bottomPadding
            leftPadding: 5
            rightPadding: 5
            text: display
            font: headerTextMetrics.font
            color: StudioTheme.Values.themeTextColor
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            anchors.fill: parent
            elide: Text.ElideRight
        }

        states: [
            State {
                name: "default"
                when: index !== selectedItem
                PropertyChanges {
                    target: headerItem
                    border.color: StudioTheme.Values.themeControlBackgroundInteraction
                }

                PropertyChanges {
                    target: headerText
                    font.bold: false
                }
            },
            State {
                name: "selected"
                when: index === selectedItem

                PropertyChanges {
                    target: headerItem
                    border.color: StudioTheme.Values.themeControlBackground
                }

                PropertyChanges {
                    target: headerText
                    font.bold: true
                }
            }
        ]
    }

    EditPropertyDialog {
        id: editProperyDialog
        model: root.model
    }

    StudioControls.Dialog {
        id: deleteColumnDialog

        property int clickedIndex: -1

        title: qsTr("Delete Column")
        width: 400

        onAccepted: {
            root.model.removeColumn(clickedIndex)
        }

        function popUp(index)
        {
            deleteColumnDialog.clickedIndex = index
            deleteColumnDialog.open()
        }

        contentItem: ColumnLayout {
            spacing: StudioTheme.Values.sectionColumnSpacing

            Text {
                text: qsTr("Are you sure that you want to delete column \"%1\"?").arg(
                           root.model.headerData(
                               deleteColumnDialog.clickedIndex, Qt.Horizontal))
                color: StudioTheme.Values.themeTextColor
            }

            RowLayout {
                Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                spacing: StudioTheme.Values.sectionRowSpacing

                HelperWidgets.Button {
                    text: qsTr("Delete")
                    onClicked: deleteColumnDialog.accept()
                }

                HelperWidgets.Button {
                    text: qsTr("Cancel")
                    onClicked: deleteColumnDialog.reject()
                }
            }
        }
    }
}
