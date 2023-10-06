// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import HelperWidgets 2.0 as HelperWidgets
import StudioTheme 1.0 as StudioTheme
import StudioControls 1.0 as StudioControls

Rectangle {
    id: root

    required property var model

    property alias leftPadding: topRow.leftPadding
    property real rightPadding: topRow.rightPadding

    color: StudioTheme.Values.themeBackgroundColorAlternate

    Column {
        id: topRow

        spacing: 0
        width: parent.width
        leftPadding: 20
        rightPadding: 0
        topPadding: 5

        Text {
            id: collectionNameText

            leftPadding: 8
            rightPadding: 8
            topPadding: 3
            bottomPadding: 3

            color: StudioTheme.Values.themeTextColor
            text: root.model.collectionName
            font.pixelSize: StudioTheme.Values.mediumIconFont
            elide: Text.ElideRight

            Rectangle {
                anchors.fill: parent
                z: parent.z - 1
                color: StudioTheme.Values.themeBackgroundColorNormal
            }
        }

        Item { // spacer
            width: 1
            height: 10
        }

        HorizontalHeaderView {
            id: headerView

            property real topPadding: 5
            property real bottomPadding: 5

            height: headerMetrics.height + topPadding + bottomPadding

            syncView: tableView
            clip: true

            delegate: Rectangle {
                id: headerItem
                implicitWidth: 100
                implicitHeight: headerText.height
                border.width: 2
                border.color: StudioTheme.Values.themeControlOutline
                clip: true

                Text {
                    id: headerText

                    topPadding: headerView.topPadding
                    bottomPadding: headerView.bottomPadding
                    leftPadding: 5
                    rightPadding: 5
                    text: display
                    font: headerMetrics.font
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    anchors.centerIn: parent
                    elide: Text.ElideRight
                }

                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.LeftButton | Qt.RightButton
                    onClicked: (mouse) => {
                        root.model.selectColumn(index)

                        if (mouse.button === Qt.RightButton)
                            headerMenu.popIndex(index)

                        mouse.accepted = true
                    }
                }

                states: [
                    State {
                        name: "default"
                        when: index !== root.model.selectedColumn
                        PropertyChanges {
                            target: headerItem
                            color: StudioTheme.Values.themeControlBackground
                        }

                        PropertyChanges {
                            target: headerText
                            color: StudioTheme.Values.themeIdleGreen
                        }
                    },
                    State {
                        name: "selected"
                        when: index === root.model.selectedColumn

                        PropertyChanges {
                            target: headerItem
                            color: StudioTheme.Values.themeControlBackgroundInteraction
                        }

                        PropertyChanges {
                            target: headerText
                            color: StudioTheme.Values.themeRunningGreen
                        }
                    }
                ]
            }

            TextMetrics {
                id: headerMetrics

                font.pixelSize: StudioTheme.Values.baseFontSize
                text: "Xq"
            }

            StudioControls.Menu {
                id: headerMenu

                property int clickedHeader: -1

                function popIndex(clickedIndex)
                {
                    headerMenu.clickedHeader = clickedIndex
                    headerMenu.popup()
                }

                onClosed: {
                    headerMenu.clickedHeader = -1
                }

                StudioControls.MenuItem {
                    text: qsTr("Edit")
                    onTriggered: editProperyDialog.editProperty(headerMenu.clickedHeader)
                }

                StudioControls.MenuItem {
                    text: qsTr("Delete")
                    onTriggered: deleteColumnDialog.popUp(headerMenu.clickedHeader)
                }
            }
        }
    }

    TableView {
        id: tableView

        anchors {
            top: topRow.bottom
            left: parent.left
            right: parent.right
            bottom: parent.bottom
            leftMargin: root.leftPadding
            rightMargin: root.rightPadding
        }

        model: root.model

        delegate: Rectangle {
            id: itemCell
            implicitWidth: 100
            implicitHeight: itemText.height
            border.width: 1
            border.color: StudioTheme.Values.themeControlOutline

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

            states: [
                State {
                    name: "default"
                    when: !itemSelected

                    PropertyChanges {
                        target: itemCell
                        color: StudioTheme.Values.themeControlBackground
                    }

                    PropertyChanges {
                        target: itemText
                        color: StudioTheme.Values.themeTextColor
                    }
                },
                State {
                    name: "selected"
                    when: itemSelected

                    PropertyChanges {
                        target: itemCell
                        color: StudioTheme.Values.themeControlBackgroundInteraction
                    }

                    PropertyChanges {
                        target: itemText
                        color: StudioTheme.Values.themeInteraction
                    }
                }
            ]
        }
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

        contentItem: Column {
            spacing: 2

            Text {
                text: qsTr("Are you sure that you want to delete column \"%1\"?").arg(
                           root.model.headerData(
                               deleteColumnDialog.clickedIndex, Qt.Horizontal))
                color: StudioTheme.Values.themeTextColor
            }

            Item { // spacer
                width: 1
                height: 20
            }

            Row {
                anchors.right: parent.right
                spacing: 10

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
