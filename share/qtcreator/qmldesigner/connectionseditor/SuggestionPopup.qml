// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls as Controls
import HelperWidgets as HelperWidgets
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
    property bool showOperators: false
    property alias operatorModel: repeater.model

    function reset() {
        search.clear()
        stack.pop(null, Controls.StackView.Immediate)
        root.listModel.reset()
    }

    closePolicy: Controls.Popup.CloseOnEscape | Controls.Popup.CloseOnPressOutsideParent
    padding: 0
    focus: search.activeFocus

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
            visible: !root.showOperators

            onSearchChanged: function(value) {
                root.treeModel.setFilter(value)
            }
        }

        Controls.StackView {
            id: stack
            visible: !root.showOperators
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
                    color: backMouseArea.containsMouse ? StudioTheme.Values.themeInteraction : "transparent"

                    MouseArea {
                        id: backMouseArea
                        anchors.fill: parent
                        hoverEnabled: true

                        onClicked: {
                            stack.pop(Controls.StackView.Immediate)
                            root.listModel.goUp()
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
                                color: backMouseArea.containsMouse ? StudioTheme.Values.themeTextSelectedTextColor
                                                                   : StudioTheme.Values.themeTextColor
                                text: StudioTheme.Constants.back_medium
                                anchors.centerIn: parent
                            }
                        }

                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: root.listModel.parentName
                            color: backMouseArea.containsMouse ? StudioTheme.Values.themeTextSelectedTextColor
                                                               : StudioTheme.Values.themeTextColor
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
                    implicitHeight: Math.min(180, childrenRect.height)
                    clip: true
                    model: root.listModel

                    HoverHandler { id: listViewHoverHandler }

                    boundsMovement: Flickable.StopAtBounds
                    boundsBehavior: Flickable.StopAtBounds

                    Controls.ScrollBar.vertical: StudioControls.TransientScrollBar {
                        id: listScrollBar
                        parent: listView
                        x: listView.width - listScrollBar.width
                        y: 0
                        height: listView.availableHeight
                        orientation: Qt.Vertical

                        show: (listViewHoverHandler.hovered || listView.focus || listScrollBar.inUse)
                              && listScrollBar.isNeeded
                    }

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
                    implicitHeight: Math.min(180, childrenRect.height)
                    clip: true
                    model: root.treeModel

                    HoverHandler { id: treeViewHoverHandler }

                    boundsMovement: Flickable.StopAtBounds
                    boundsBehavior: Flickable.StopAtBounds

                    Controls.ScrollBar.vertical: StudioControls.TransientScrollBar {
                        id: treeScrollBar
                        parent: treeView
                        x: treeView.width - treeScrollBar.width
                        y: 0
                        height: treeView.availableHeight
                        orientation: Qt.Vertical

                        show: (treeViewHoverHandler.hovered || treeView.focus || treeScrollBar.inUse)
                              && treeScrollBar.isNeeded
                    }

                    onLayoutChanged: function() {
                        treeView.expand(0)
                    }

                    rowHeightProvider: function(row) {
                        return (row <= 0) ? 0 : -1
                    }

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
            visible: root.showOperators
            width: stack.width
            height: flow.childrenRect.height + 2 * StudioTheme.Values.flowMargin

            Flow {
                id: flow

                anchors.fill: parent
                anchors.margins: StudioTheme.Values.flowMargin
                spacing: StudioTheme.Values.flowSpacing

                Repeater {
                    id: repeater

                    Rectangle {
                        id: delegate

                        required property int index

                        required property string name
                        required property string value
                        required property string tooltip

                        width: textItem.contentWidth + 2 * StudioTheme.Values.flowPillMargin
                        height: StudioTheme.Values.flowPillHeight
                        color: mouseArea.containsMouse ? StudioTheme.Values.themePillOperatorBackgroundHover
                                                       : StudioTheme.Values.themePillOperatorBackgroundIdle
                        radius: StudioTheme.Values.flowPillRadius

                        HelperWidgets.ToolTipArea {
                            id: mouseArea
                            hoverEnabled: true
                            anchors.fill: parent
                            tooltip: delegate.tooltip

                            onClicked: root.select(delegate.value)
                        }

                        Text {
                            id: textItem
                            font.pixelSize: StudioTheme.Values.baseFontSize
                            color: StudioTheme.Values.themePillText
                            text: delegate.name
                            anchors.centerIn: parent
                        }
                    }
                }
            }
        }
    }
}
