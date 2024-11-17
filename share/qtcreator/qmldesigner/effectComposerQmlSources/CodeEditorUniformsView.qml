// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import StudioControls as StudioControls
import StudioTheme as StudioTheme
import TableModules as TableModules

ColumnLayout {
    property alias model: tableView.model
    readonly property alias tableView: tableView

    spacing: -1

    HoverHandler { id: hoverHandler }

    HorizontalHeaderView {
        id: horizontalHeader

        Layout.fillWidth: true

        syncView: tableView
        clip: true
        interactive: false

        delegate: Rectangle {
            color: StudioTheme.Values.themePanelBackground
            implicitWidth: StudioTheme.Values.cellWidth
            implicitHeight: StudioTheme.Values.cellHeight
            border {
                width: StudioTheme.Values.border
                color: StudioTheme.Values.themeStateSeparator
            }

            Text {
                color: StudioTheme.Values.themeTextColor
                text: display
                anchors.fill: parent
                anchors.margins: 8
                elide: Text.ElideRight
                font.bold: true

                verticalAlignment: Qt.AlignVCenter
            }
        }
    }

    Item {
        Layout.fillWidth: true
        Layout.fillHeight: true

        ScrollView {
            id: scrollView

            anchors.fill: parent

            contentItem: TableView {
                id: tableView

                columnSpacing: -StudioTheme.Values.border
                rowSpacing: -StudioTheme.Values.border
                clip: true
                interactive: false
                selectionMode: TableView.SingleSelection
                selectionBehavior: TableView.SelectRows
                selectionModel: ItemSelectionModel {}
                delegate: Cell {
                    id: dataScope

                    Text {
                        id: labelView

                        text: dataScope.display ?? ""
                        visible: !dataScope.editing
                        color: StudioTheme.Values.themeTextColor
                        anchors.fill: parent
                        anchors.margins: 8
                        elide: Text.ElideMiddle
                        wrapMode: Text.WordWrap
                        maximumLineCount: 1
                        verticalAlignment: Qt.AlignVCenter
                        clip: true

                        StudioControls.ToolTipArea {
                            anchors.fill: parent
                            text: labelView.text
                            enabled: labelView.truncated
                        }

                        Loader {
                            active: dataScope.canCopy

                            anchors.right: parent.right
                            anchors.top: parent.top
                            anchors.bottom: parent.bottom

                            sourceComponent: MouseArea {
                                id: hoverArea

                                width: 15
                                hoverEnabled: true
                                enabled: true

                                Row {
                                    anchors.fill: parent
                                    visible: hoverArea.containsMouse

                                    StudioControls.AbstractButton {
                                        width: iconSize
                                        height: iconSize
                                        anchors.verticalCenter: parent.verticalCenter
                                        buttonIcon: StudioTheme.Constants.copy_small
                                        backgroundVisible: false
                                        onClicked: rootView.copyText(dataScope.display)
                                    }

                                    // ToDo: Add a button for placing the value to the editor
                                }
                            }
                        }
                    }
                }
            }

            ScrollBar.horizontal: StudioControls.TransientScrollBar {
                id: horizontalScrollBar

                style: StudioTheme.Values.viewStyle
                parent: tableView
                x: 0
                y: tableView.height - horizontalScrollBar.height
                width: tableView.availableWidth - (verticalScrollBar.isNeeded ? verticalScrollBar.thickness : 0)
                orientation: Qt.Horizontal

                visible: !tableView.hideHorizontalScrollBar

                show: (hoverHandler.hovered || tableView.focus || tableView.adsFocus
                       || horizontalScrollBar.inUse || horizontalScrollBar.otherInUse)
                      && horizontalScrollBar.isNeeded
                otherInUse: verticalScrollBar.inUse
            }

            ScrollBar.vertical: StudioControls.TransientScrollBar {
                id: verticalScrollBar

                style: StudioTheme.Values.viewStyle
                parent: tableView
                x: tableView.width - verticalScrollBar.width
                y: 0
                height: tableView.availableHeight - (horizontalScrollBar.isNeeded ? horizontalScrollBar.thickness : 0)
                orientation: Qt.Vertical

                visible: !tableView.hideVerticalScrollBar

                show: (hoverHandler.hovered || tableView.focus || tableView.adsFocus
                       || horizontalScrollBar.inUse || horizontalScrollBar.otherInUse)
                      && verticalScrollBar.isNeeded
                otherInUse: horizontalScrollBar.inUse
            }
        }
    }

    component Cell: Rectangle {
        id: cell

        required property var display
        required property int row
        required property int column

        required property bool editing
        required property bool selected
        required property bool current
        required property bool canCopy

        color: tableView.currentRow === row ? StudioTheme.Values.themeTableCellCurrent : StudioTheme.Values.themePanelBackground
        implicitWidth: StudioTheme.Values.cellWidth
        implicitHeight: StudioTheme.Values.cellHeight
        border {
            width: StudioTheme.Values.border
            color: StudioTheme.Values.themeStateSeparator
        }
    }
}
