// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import CollectionDetails 1.0 as CollectionDetails
import HelperWidgets 2.0 as HelperWidgets
import StudioControls 1.0 as StudioControls
import StudioTheme 1.0 as StudioTheme

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

        visible: root.model.collectionName !== ""

        spacing: 0
        anchors.fill: parent

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

            Layout.leftMargin: StudioTheme.Values.collectionTableHorizontalMargin
            Layout.topMargin: StudioTheme.Values.collectionTableVerticalMargin

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
                    selectedItem: tableView.model.selectedColumn
                    color: StudioTheme.Values.themeControlBackgroundInteraction

                    MouseArea {
                        id: topHeaderMouseArea

                        anchors.fill: parent
                        anchors.margins: 5
                        acceptedButtons: Qt.LeftButton | Qt.RightButton
                        hoverEnabled: true
                        onClicked: (mouse) => {
                            tableView.model.selectColumn(index)

                            if (mouse.button === Qt.RightButton) {
                                let posX = index === root.model.columnCount() - 1 ? parent.width - editPropertyDialog.width : 0

                                headerMenu.clickedHeaderIndex = index
                                headerMenu.dialogPos = parent.mapToGlobal(posX, parent.height)
                                headerMenu.popup()
                            } else {
                                headerMenu.close()
                            }
                        }
                    }

                    ToolTip {
                        id: topHeaderToolTip

                        property bool expectedToBeShown: topHeaderMouseArea.containsMouse
                        visible: expectedToBeShown && text !== ""
                        delay: 1000

                        onExpectedToBeShownChanged: {
                            if (expectedToBeShown)
                                text = root.model.propertyType(index)
                        }
                    }
                }

                StudioControls.Menu {
                    id: headerMenu

                    property int clickedHeaderIndex: -1
                    property point dialogPos

                    onClosed: {
                        headerMenu.clickedHeaderIndex = -1
                    }

                    StudioControls.MenuItem {
                        text: qsTr("Edit")
                        onTriggered: editPropertyDialog.openDialog(headerMenu.clickedHeaderIndex,
                                                                   headerMenu.dialogPos)
                    }

                    StudioControls.MenuItem {
                        text: qsTr("Delete")
                        onTriggered: deleteColumnDialog.popUp(headerMenu.clickedHeaderIndex)
                    }

                    StudioControls.MenuItem {
                        text: qsTr("Sort Ascending")
                        onTriggered: sortedModel.sort(headerMenu.clickedHeaderIndex, Qt.AscendingOrder)
                    }

                    StudioControls.MenuItem {
                        text: qsTr("Sort Descending")
                        onTriggered: sortedModel.sort(headerMenu.clickedHeaderIndex, Qt.DescendingOrder)
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

                    clip: true
                    implicitWidth: 100
                    implicitHeight: StudioTheme.Values.baseHeight
                    color: itemSelected ? StudioTheme.Values.themeControlBackgroundInteraction
                                        : StudioTheme.Values.themeControlBackground
                    border.width: 1
                    border.color: {
                        if (dataTypeWarning !== CollectionDetails.Warning.None)
                            return StudioTheme.Values.themeWarning

                        if (itemSelected)
                            return StudioTheme.Values.themeControlOutlineInteraction

                        return StudioTheme.Values.themeControlBackgroundInteraction
                    }

                    HelperWidgets.ToolTipArea {
                        anchors.fill: parent
                        text: root.model.warningToString(dataTypeWarning)
                        enabled: dataTypeWarning !== CollectionDetails.Warning.None && text !== ""
                        hoverEnabled: true
                        acceptedButtons: Qt.NoButton
                    }

                    MouseArea {
                        anchors.fill: parent
                        acceptedButtons: Qt.RightButton
                        onClicked: (mouse) => {
                            let row = index % tableView.model.rowCount()

                            tableView.model.selectRow(row)
                            cellContextMenu.popup()
                        }
                    }

                    Loader {
                        id: cellContentLoader

                        property int cellColumnType: columnType ? columnType : 0

                        Component {
                            id: cellText

                            Text {
                                text: display ?? ""
                                color: itemSelected ? StudioTheme.Values.themeInteraction
                                                    : StudioTheme.Values.themePlaceholderTextColorInteraction
                                leftPadding: 5
                                topPadding: 3
                                bottomPadding: 3
                                font.pixelSize: StudioTheme.Values.baseFontSize
                                horizontalAlignment: Text.AlignLeft
                                verticalAlignment: Text.AlignVCenter
                                elide: Text.ElideRight
                            }
                        }

                        Component {
                            id: colorEditorComponent

                            ColorViewDelegate {}
                        }

                        function resetSource() {
                            if (columnType === CollectionDetails.DataType.Color)
                                cellContentLoader.sourceComponent = colorEditorComponent
                            else
                                cellContentLoader.sourceComponent = cellText
                        }

                        Component.onCompleted: resetSource()
                        onCellColumnTypeChanged: resetSource()
                    }

                    TableView.editDelegate: CollectionDetailsEditDelegate {
                        anchors {
                            top: itemCell.top
                            left: itemCell.left
                        }
                    }

                    StudioControls.Menu {
                        id: cellContextMenu

                        width: 140

                        StudioControls.MenuItem {
                            HelperWidgets.IconLabel {
                                icon: StudioTheme.Constants.addrowabove_medium

                                anchors.left: parent.left
                                anchors.leftMargin: 10
                                anchors.verticalCenter: parent.verticalCenter

                                Text {
                                    text: qsTr("Add row above")
                                    color: StudioTheme.Values.themeTextColor
                                    anchors.left: parent.right
                                    anchors.leftMargin: 10
                                }
                            }

                            onTriggered: root.model.insertRow(root.model.selectedRow)
                        }

                        StudioControls.MenuItem {
                            HelperWidgets.IconLabel {
                                icon: StudioTheme.Constants.addrowabove_medium

                                anchors.left: parent.left
                                anchors.leftMargin: 10
                                anchors.verticalCenter: parent.verticalCenter

                                Text {
                                    text: qsTr("Add row below")
                                    color: StudioTheme.Values.themeTextColor
                                    anchors.left: parent.right
                                    anchors.leftMargin: 10
                                }
                            }

                            onTriggered: root.model.insertRow(root.model.selectedRow + 1)
                        }

                        StudioControls.MenuItem {
                            HelperWidgets.IconLabel {
                                icon: StudioTheme.Constants.addrowabove_medium

                                anchors.left: parent.left
                                anchors.leftMargin: 10
                                anchors.verticalCenter: parent.verticalCenter

                                Text {
                                    text: qsTr("Delete this row")
                                    color: StudioTheme.Values.themeTextColor
                                    anchors.left: parent.right
                                    anchors.leftMargin: 10
                                }
                            }

                            onTriggered: root.model.removeRows(root.model.selectedRow, 1)
                        }
                    }
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
        anchors.centerIn: parent
        text: qsTr("Select a model to continue")
        visible: !topRow.visible
        color: StudioTheme.Values.themeTextColor
        font.pixelSize: StudioTheme.Values.mediumFontSize
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
        id: editPropertyDialog
        model: root.model
    }

    Connections {
        target: root.parent
        function onIsHorizontalChanged() { editPropertyDialog.close() }
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
