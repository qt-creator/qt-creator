// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import StudioControls as StudioControls
import StudioTheme as StudioTheme
import TableModules as TableModules

ColumnLayout {
    id: root
    property alias model: tableView.model
    readonly property alias tableView: tableView
    property var headerImplicitWidths: []

    spacing: -1

    HoverHandler { id: hoverHandler }

    CellFontMetrics {
        id: headerFontMetrics

        font.bold: true
    }

    CellFontMetrics {
        id: cellFontMetrics
    }

    HorizontalHeaderView {
        id: horizontalHeader

        Layout.fillWidth: true

        syncView: tableView
        clip: true
        interactive: false

        delegate: Rectangle {
            color: StudioTheme.Values.themePanelBackground
            implicitHeight: StudioTheme.Values.cellHeight
            implicitWidth: headerText.firstLineWidth

            border {
                width: StudioTheme.Values.border
                color: StudioTheme.Values.themeStateSeparator
            }

            Text {
                id: headerText

                readonly property real firstLineWidth: headerFontMetrics.getWidth(text)
                                                       + rightPadding
                                                       + leftPadding
                                                       + anchors.leftMargin
                                                       + anchors.rightMargin
                                                       + 2

                color: StudioTheme.Values.themeTextColor
                text: display
                anchors.fill: parent
                anchors.margins: 8
                elide: Text.ElideRight
                font: headerFontMetrics.font

                verticalAlignment: Qt.AlignVCenter

                onFirstLineWidthChanged: {
                    root.headerImplicitWidths[column] = headerText.firstLineWidth
                }
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
                interactive: true
                selectionMode: TableView.SingleSelection
                selectionBehavior: TableView.SelectRows
                selectionModel: ItemSelectionModel {}

                delegate: Cell {
                    id: dataScope

                    property real headerItemWidth: root.headerImplicitWidths[column] ?? 50

                    implicitWidth: Math.max(labelView.firstLineWidth, headerItemWidth)

                    Binding on implicitWidth {
                        when: dataScope.isDescription && dataScope.isLastCol
                        value: root.width - dataScope.x
                        restoreMode: Binding.RestoreBinding
                    }

                    Text {
                        id: labelView

                        readonly property real firstLineWidth: cellFontMetrics.getWidth(text)
                                                               + rightPadding
                                                               + leftPadding
                                                               + anchors.leftMargin
                                                               + anchors.rightMargin
                                                               + 2

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
                        rightPadding: cellButtonsLoader.width
                        font: cellFontMetrics.font

                        StudioControls.ToolTipArea {
                            anchors.fill: parent
                            text: labelView.text
                            enabled: labelView.truncated
                        }

                        Loader {
                            id: cellButtonsLoader
                            active: dataScope.canCopy

                            anchors.right: parent.right
                            anchors.top: parent.top
                            anchors.bottom: parent.bottom

                            sourceComponent: MouseArea {
                                id: hoverArea

                                width: buttonsRow.implicitWidth
                                hoverEnabled: true
                                enabled: true

                                Row {
                                    id: buttonsRow

                                    anchors.fill: parent
                                    spacing: StudioTheme.Values.controlGap
                                    visible: hoverArea.containsMouse
                                    leftPadding: 5

                                    CellButton {
                                        buttonIcon: StudioTheme.Constants.assignTo_medium
                                        onClicked: rootEditor.insertTextToCursorPosition(dataScope.display)
                                        tooltip: qsTr("Insert into the editor cursor position.")
                                    }

                                    CellButton {
                                        buttonIcon: StudioTheme.Constants.copy_small
                                        onClicked: rootEditor.copyText(dataScope.display)
                                        tooltip: qsTr("Copy uniform name to clipboard.")
                                    }
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

                show: (hoverHandler.hovered || tableView.focus
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

                show: (hoverHandler.hovered || tableView.focus
                       || verticalScrollBar.inUse || verticalScrollBar.otherInUse)
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
        required property bool isDescription

        readonly property bool isLastCol: column === tableView.columns - 1

        color: tableView.currentRow === row ? StudioTheme.Values.themeTableCellCurrent : StudioTheme.Values.themePanelBackground
        implicitWidth: 120
        implicitHeight: 27
        border {
            width: StudioTheme.Values.border
            color: StudioTheme.Values.themeStateSeparator
        }
    }

    component CellButton: StudioControls.AbstractButton {
        id: cellBtn

        property alias tooltip: cellBtnTooltip.text

        width: iconSize
        height: iconSize
        anchors.verticalCenter: parent.verticalCenter
        buttonIcon: StudioTheme.Constants.assignTo_medium
        iconSize: StudioTheme.Values.miniIcon
        backgroundVisible: false

        StudioControls.ToolTip {
            id: cellBtnTooltip

            visible: cellBtn.hovered && text !== ""
        }
    }

    component CellFontMetrics: FontMetrics {
        id: textFont

        function getWidth(txt) {
            let nIdx = txt.indexOf('\n')
            let firstLineText = (nIdx > -1) ? txt.substr(0, nIdx) : txt
            return Math.ceil(textFont.boundingRect(firstLineText).width)
        }
    }
}
