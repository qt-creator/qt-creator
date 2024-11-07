// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Shapes
import QtQuick.Templates as T
import Qt.labs.qmlmodels

import StudioControls as StudioControls
import StudioTheme as StudioTheme

import DeviceManagerControls as DMC

Rectangle {
    id: root
    color: StudioTheme.Values.themePanelBackground

    readonly property int sideBarWidth: 50
    readonly property int sideBarExpandedWidth: 350

    readonly property int popupMargin: 6

    component Cell: Rectangle {
        required property var display
        required property int row
        required property int column

        required property bool editing
        required property bool selected
        required property bool current

        color: tableView.currentRow === row ? "#08475B" : StudioTheme.Values.themePanelBackground
        implicitWidth: StudioTheme.Values.cellWidth
        implicitHeight: StudioTheme.Values.cellHeight
        border {
            width: StudioTheme.Values.border
            color: StudioTheme.Values.themeStateSeparator
        }
    }

    component CustomPopup: T.Popup {
        id: popup

        function openAt(item: Item) {
            popup.open() // open first so popup position is writable

            let rootRectangle = row.mapToItem(root, item.x, item.y, item.width, item.height)

            popup.x = Math.max(root.popupMargin,
                               Math.min(rootRectangle.x - popup.width / 2,
                                        root.width - popup.width - root.popupMargin))
            popup.y = rootRectangle.y + rootRectangle.height + popup.chevronSize.height

            popup.chevronPosition = Qt.point(rootRectangle.x - popup.x + rootRectangle.width / 2,
                                            -popup.chevronSize.height)
        }

        implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                                contentWidth + leftPadding + rightPadding)
        implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                                 contentHeight + topPadding + bottomPadding)

        property point chevronPosition: Qt.point(0, 0)
        property size chevronSize: Qt.size(20, 10)

        padding: 20
        modal: false
        focus: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        background: Rectangle {
            color: StudioTheme.Values.themePopupBackground
            radius: 4

            Shape {
                id: chevron

                ShapePath {
                    id: path
                    strokeWidth: -1
                    fillColor: StudioTheme.Values.themePopupBackground
                    startX: popup.chevronPosition.x - popup.chevronSize.width / 2
                    startY: popup.chevronPosition.y + popup.chevronSize.height

                    PathLine {
                        id: peak
                        x: popup.chevronPosition.x
                        y: popup.chevronPosition.y
                    }

                    PathLine {
                        id: end
                        x: popup.chevronPosition.x + popup.chevronSize.width / 2
                        y: popup.chevronPosition.y + popup.chevronSize.height
                    }
                }
            }
        }
    }

    DelegateChooser {
        id: chooser

        DelegateChoice {
            column: DeviceManagerModel.Status

            Cell {
                id: statusDelegate

                Rectangle {
                    id: statusBackground

                    anchors.centerIn: parent

                    width: 60
                    height: 30
                    radius: 10
                    color: statusDelegate.display === DeviceManagerModel.Online ? "#1AAA55"
                                                                                : "#c98014"

                    Text {
                        color: StudioTheme.Values.themeTextColor
                        text: statusDelegate.display === DeviceManagerModel.Online ? qsTr("Online")
                                                                                   : qsTr("Offline")
                        anchors.fill: parent
                        anchors.margins: 8

                        horizontalAlignment: Qt.AlignHCenter
                        verticalAlignment: Qt.AlignVCenter
                    }
                }
            }
        }

        DelegateChoice {
            column: DeviceManagerModel.Active

            Cell {
                id: activeDelegate

                DMC.Switch {
                    checked: activeDelegate.display === true
                    anchors.centerIn: parent

                    onToggled: {
                        let index = activeDelegate.TableView.view.index(activeDelegate.row, activeDelegate.column)
                        activeDelegate.TableView.view.model.setData(index, checked, Qt.EditRole)
                    }
                }
            }
        }

        DelegateChoice {
            column: DeviceManagerModel.Alias

            Cell {
                id: aliasDelegate

                Text {
                    text: aliasDelegate.display
                    visible: !aliasDelegate.editing
                    color: StudioTheme.Values.themeTextColor
                    anchors.fill: parent
                    anchors.margins: 8
                    elide: Text.ElideMiddle

                    horizontalAlignment: Qt.AlignLeft
                    verticalAlignment: Qt.AlignVCenter
                }

                TableView.editDelegate: TextField {
                    anchors.fill: parent
                    text: aliasDelegate.display
                    horizontalAlignment: TextInput.AlignLeft
                    verticalAlignment: TextInput.AlignVCenter
                    padding: 8
                    Component.onCompleted: selectAll()

                    TableView.onCommit: {
                        let index = aliasDelegate.TableView.view.index(aliasDelegate.row, aliasDelegate.column)
                        aliasDelegate.TableView.view.model.setData(index, text, Qt.EditRole)
                    }

                    background: Rectangle {
                        color: StudioTheme.Values.themePanelBackground
                        border.color: StudioTheme.Values.themeInteraction
                        border.width: StudioTheme.Values.border
                    }
                }
            }
        }

        DelegateChoice {
            Cell {
                Text {
                    color: StudioTheme.Values.themeTextColor
                    text: display
                    anchors.fill: parent
                    anchors.margins: 8
                    elide: Text.ElideMiddle

                    horizontalAlignment: Qt.AlignLeft
                    verticalAlignment: Qt.AlignVCenter
                }
            }
        }
    }

    CustomPopup {
        id: questionPopup

        contentItem: Item {
            implicitWidth: 400
            implicitHeight: questionColumnLayout.height

            ColumnLayout {
                id: questionColumnLayout
                anchors.left: parent.left
                anchors.right: parent.right

                Text {
                    Layout.fillWidth: true
                    text: qsTr("How to see a preview on Android device")
                    color: StudioTheme.Values.themeTextColor
                    wrapMode: Text.WordWrap
                    font.bold: true
                }

                component CollationItem: RowLayout {
                    id: collationItem
                    Layout.fillWidth: true
                    spacing: 6

                    property int number: 0
                    property alias text: textLabel.text

                    Label {
                        id: numberLabel
                        Layout.alignment: Qt.AlignTop
                        text: collationItem.number + "."
                    }

                    Label {
                        id: textLabel
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                    }
                }

                CollationItem {
                    number: 1
                    text: qsTr("Scan the QR code below or click on the link to go to the Google Play store with your device and seek for Qt Viewer application.")
                }

                RowLayout {
                    Layout.fillWidth: true

                    Image {
                        Layout.maximumWidth: 200
                        Layout.alignment: Qt.AlignHCenter
                        fillMode: Image.PreserveAspectFit
                        source: "images/GetItOnGooglePlay_Badge_Web_color_English.png"

                        MouseArea {}
                    }

                    Image {
                        id: playstoreQR

                        property string payload: JSON.stringify({
                            background: `${StudioTheme.Values.themePopupBackground}`,
                            foreground: `${StudioTheme.Values.themeTextColor}`,
                            content: "https://play.google.com/store/apps/details?id=io.qt.qtdesignviewer"
                        })

                        Layout.maximumWidth: 200
                        Layout.alignment: Qt.AlignHCenter
                        // Requested size
                        sourceSize.width: 200
                        sourceSize.height: 200
                        fillMode: Image.PreserveAspectFit
                        source: "image://QrGenerator/" + playstoreQR.payload
                    }
                }

                CollationItem {
                    number: 2
                    text: qsTr("Install the Qt Viewer application on your phone.")
                }

                CollationItem {
                    number: 3
                    text: qsTr("Open up this window again, if you already closed it, and click on \"Add Run Target\" button in this window.")
                }
            }
        }
    }

    ColumnLayout {
        id: column
        anchors.fill: parent

        Rectangle {
            id: toolbar
            color: StudioTheme.Values.themeToolbarBackground

            Layout.fillWidth: true
            Layout.preferredHeight: StudioTheme.Values.toolbarHeight

            RowLayout {
                id: row
                anchors.fill: parent
                anchors.topMargin: StudioTheme.Values.toolbarVerticalMargin
                anchors.bottomMargin: StudioTheme.Values.toolbarVerticalMargin
                anchors.leftMargin: StudioTheme.Values.toolbarHorizontalMargin
                anchors.rightMargin: StudioTheme.Values.toolbarHorizontalMargin
                spacing: 6

                function addRunTarget(ip: string) {
                    let added = DeviceManagerBackend.deviceManagerModel.addDevice(ip)
                    if (added)
                        ipInput.clear()
                }

                DMC.ButtonInput {
                    id: ipInput

                    width: 200
                    buttonIcon: StudioTheme.Constants.add_medium
                    placeholderText: qsTr("Run target IP")
                    validator: RegularExpressionValidator {
                        regularExpression: /^(\d{1,3}\.){3}\d{1,3}$/
                    }

                    tooltip: qsTr("Add Run Target")

                    style: StudioTheme.SearchControlStyle {}

                    onButtonClicked: row.addRunTarget(ipInput.text)
                    onAccepted: row.addRunTarget(ipInput.text)
                }

                DMC.ToolbarButton {
                    id: removeRunTargetButton
                    enabled: tableView.currentRow !== -1
                    buttonIcon: StudioTheme.Constants.remove_medium
                    tooltip: qsTr("Remove Run Target")

                    onClicked: {
                        let index = tableView.index(tableView.currentRow, DeviceManagerModel.DeviceId)
                        let deviceId = tableView.model.data(index, Qt.DisplayRole)
                        DeviceManagerBackend.deviceManagerModel.removeDevice(deviceId)
                    }
                }

                Item {
                    Layout.fillWidth: true
                }

                DMC.Dropdown {
                    text: qsTr("Columns")
                    model: ListModel {
                        id: columnModel
                        onDataChanged: {
                            tableView.forceLayout()
                        }
                    }
                    style: StudioTheme.Values.viewBarControlStyle

                    Component.onCompleted: {
                        tableView.setColumnWidth(DeviceManagerModel.Status, 80)
                        tableView.setColumnWidth(DeviceManagerModel.Active, 70)
                        tableView.setColumnWidth(DeviceManagerModel.Alias, 200)
                        tableView.setColumnWidth(DeviceManagerModel.IPv4Addr, 110)
                        tableView.setColumnWidth(DeviceManagerModel.OS, 100)
                        tableView.setColumnWidth(DeviceManagerModel.OSVersion, 100)
                        tableView.setColumnWidth(DeviceManagerModel.Architecture, 100)
                        tableView.setColumnWidth(DeviceManagerModel.ScreenSize, 90)
                        //tableView.setColumnWidth(DeviceManagerModel.AppVersion, 130)
                        tableView.setColumnWidth(DeviceManagerModel.DeviceId, 250)

                        for (let i = 0; i < DeviceManagerBackend.deviceManagerModel.columnCount(); ++i) {
                            columnModel.append({"name": DeviceManagerBackend.deviceManagerModel.headerData(i, Qt.Horizontal),
                                                "hidden": false})
                        }
                    }
                }

                DMC.ToolbarButton {
                    id: questionButton
                    buttonIcon: StudioTheme.Constants.help
                    onClicked: questionPopup.openAt(questionButton)
                    checkable: true
                    checked: questionPopup.visible
                }
            }
        }

        ColumnLayout {
            id: content

            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: 20

            spacing: 20

            Text {
                id: topBar
                text: qsTr("Manage target devices to be used to display the project")
                Layout.fillWidth: true
                verticalAlignment: Text.AlignTop
                height: 20
                color: StudioTheme.Values.themeTextColor
                font.pixelSize: StudioTheme.Values.baseFontSize
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true

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

                            horizontalAlignment: Qt.AlignLeft
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

                            columnWidthProvider: function(column) {
                                if (columnModel.get(column).hidden)
                                    return 0

                                const width = explicitColumnWidth(column)
                                if (width === 0)
                                    return 0
                                else if (width > 0)
                                    return width
                                return implicitColumnWidth(column)
                            }

                            columnSpacing: -StudioTheme.Values.border
                            rowSpacing: -StudioTheme.Values.border
                            clip: true
                            interactive: false
                            selectionMode: TableView.SingleSelection
                            selectionBehavior: TableView.SelectRows
                            selectionModel: ItemSelectionModel {}
                            model: DeviceManagerBackend.deviceManagerModel
                            delegate: chooser
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
            }
        }
    }
}
