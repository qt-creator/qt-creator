// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls as Controls
import StudioTheme as StudioTheme
import StudioControls as StudioControls
import ConnectionsEditorEditorBackend

Controls.Popup {
    id: root

    property StudioTheme.ControlStyle style: StudioTheme.Values.controlStyle

    property var listModel: ConnectionsEditorEditorBackend.connectionModel.delegate.propertyListProxyModel
    property var treeModel: ConnectionsEditorEditorBackend.connectionModel.delegate.propertyTreeModel

    signal select(var value)
    signal entered(var value)
    signal exited(var value)

    property alias searchActive: search.activeFocus

    function reset() {
        search.clear()
        stack.pop(null, Controls.StackView.Immediate)
        root.listModel.reset()
    }

    closePolicy: Controls.Popup.NoAutoClose
    padding: 0

    background: Rectangle {
        implicitWidth: root.width
        color: root.style.background.idle
        border {
            color: root.style.border.idle
            width: root.style.borderWidth
        }
    }

    contentItem: Column {
        StudioControls.SearchBox {
            id: search
            width: parent.width

            onSearchChanged: function(value) {
                root.treeModel.setFilter(value)
            }
        }

        Controls.StackView {
            id: stack

            width: parent.width
            height: currentItem?.implicitHeight

            clip: true

            initialItem: mainView
        }

        Component {
            id: mainView

            Column {
                Rectangle  {
                    width: stack.width
                    height: 30
                    visible: root.listModel.parentName !== ""
                    color: backMouseArea.containsMouse ? "#4DBFFF" : "transparent"

                    MouseArea {
                        id: backMouseArea
                        anchors.fill: parent
                        hoverEnabled: true

                        onClicked: {
                            stack.pop(Controls.StackView.Immediate)
                            root.listModel.goUp() //treeModel.pop()
                        }
                    }

                    Row {
                        anchors.fill: parent

                        Item {
                            width: 30
                            height: 30

                            Text {
                                id: chevronLeft
                                font.family: StudioTheme.Constants.iconFont.family
                                font.pixelSize: root.style.baseIconFontSize
                                color: backMouseArea.containsMouse ? "#111111" : "white" // TODO colors
                                text: StudioTheme.Constants.back_medium
                                anchors.centerIn: parent
                            }
                        }

                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: root.listModel.parentName
                            color: backMouseArea.containsMouse ? "#111111" : "white" // TODO colors
                            horizontalAlignment: Text.AlignLeft
                            verticalAlignment: Text.AlignVCenter
                            elide: Text.ElideRight
                        }
                    }
                }

                Rectangle {
                    width: stack.width - 8
                    height: 1
                    visible: root.listModel.parentName !== ""
                    color: "#3C3C3C"
                    anchors.horizontalCenter: parent.horizontalCenter
                }

                ListView {
                    id: listView
                    visible: search.empty
                    width: stack.width
                    implicitHeight: Math.min(380, childrenRect.height)
                    clip: true
                    model: root.listModel

                    delegate: MyListViewDelegate {
                        id: listViewDelegate

                        required property int index

                        required property string propertyName
                        required property int childCount
                        required property string expression

                        text: listViewDelegate.propertyName
                        implicitWidth: listView.width

                        onClicked: {
                            if (!listViewDelegate.childCount) {
                                root.select(listViewDelegate.expression)
                                return
                            }

                            stack.push(mainView, Controls.StackView.Immediate)

                            ListView.view.model.goInto(listViewDelegate.index)
                        }

                        onHoveredChanged: {
                            if (listViewDelegate.childCount)
                                return

                            if (listViewDelegate.hovered)
                                root.entered(listViewDelegate.expression)
                            else
                                root.exited(listViewDelegate.expression)
                        }
                    }
                }

                TreeView {
                    id: treeView
                    visible: !search.empty
                    width: stack.width
                    implicitHeight: Math.min(380, childrenRect.height)
                    clip: true
                    model: root.treeModel

                    // This is currently a workaround and should be cleaned up. Calling
                    // expandRecursively every time the filter changes is performance wise not good.
                    //Connections {
                    //    target: proxyModel
                    //    function onFilterChanged() { treeView.expandRecursively() }
                    //}

                    delegate: MyTreeViewDelegate {
                        id: treeViewDelegate

                        required property int index

                        required property string propertyName
                        required property int childCount
                        required property string expression

                        text: treeViewDelegate.propertyName
                        implicitWidth: treeView.width

                        onClicked: {
                            if (!treeViewDelegate.childCount)
                                root.select(treeViewDelegate.expression)
                            else
                                treeView.toggleExpanded(treeViewDelegate.index)
                        }

                        onHoveredChanged: {
                            if (treeViewDelegate.childCount)
                                return

                            if (treeViewDelegate.hovered)
                                root.entered(treeViewDelegate.expression)
                            else
                                root.exited(treeViewDelegate.expression)
                        }
                    }
                }
            }
        }

        Item {
            visible: false
            width: stack.width
            height: flow.childrenRect.height + 2 * StudioTheme.Values.flowMargin

            Flow {
                id: flow

                anchors.fill: parent
                anchors.margins: StudioTheme.Values.flowMargin
                spacing: StudioTheme.Values.flowSpacing

                Repeater {
                    id: repeater

                    // TODO actual value + tooltip
                    model: ["AND", "OR", "equal", "not equal", "greater", "less", "greater then", "less then"]

                    Rectangle {
                        width: textItem.contentWidth + 14
                        height: 26
                        color: "#161616"
                        radius: 4
                        border {
                            color: "white"
                            width: mouseArea.containsMouse ? 1 : 0
                        }

                        MouseArea {
                            id: mouseArea
                            hoverEnabled: true
                            anchors.fill: parent
                        }

                        Text {
                            id: textItem
                            font.pixelSize: 12
                            color: "white"
                            text: modelData
                            anchors.centerIn: parent
                        }
                    }
                }
            }
        }
    }
}
