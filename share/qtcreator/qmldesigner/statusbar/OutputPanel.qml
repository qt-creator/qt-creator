// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import StudioControls as StudioControls
import StudioTheme as StudioTheme

import OutputPane

ScrollView {
    id: root

    property int unreadMessages: 0

    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

    function clearOutput() {
        parentListModel.resetModel()
        root.markMessagesRead()
    }

    onVisibleChanged: {
        if (root.visible === true)
            root.markMessagesRead()
    }

    function markMessagesRead() { root.unreadMessages = 0 }

    clip: true

    ScrollBar.vertical: StudioControls.TransientScrollBar {
        id: verticalScrollBar
        style: StudioTheme.Values.viewStyle
        parent: root
        x: root.width - verticalScrollBar.width
        y: 0
        height: root.availableHeight
        orientation: Qt.Vertical
        show: (root.hovered || root.focus) && verticalScrollBar.isNeeded
    }

    Column {
        Repeater {
            id: parentList

            model: AppOutputParentModel {
                id: parentListModel
                historyColor: "grey"
                messageColor: "#007b7b"
                errorColor: "#ff6666"

                onMessageAdded: {
                    if (!root.visible)
                        root.unreadMessages++
                }
            }

            delegate: ColumnLayout {
                id: parentDelegate
                spacing: 0

                required property int index
                required property string run
                required property color blockColor

                Text {
                    id: timeStampText
                    text: parentDelegate.run
                    color: parentDelegate.blockColor
                }

                Repeater {
                    id: childList

                    model: AppOutputChildModel {
                        id: childListModel
                        parentModel: parentListModel
                        row: parentDelegate.index
                    }

                    onItemAdded: verticalScrollBar.position = 1.0 // Scroll to bottom

                    delegate: Column {
                        id: childDelegate

                        required property string message
                        required property color messageColor

                        Text {
                            wrapMode: Text.WordWrap
                            text: childDelegate.message
                            color: childDelegate.messageColor
                            width: root.width
                        }
                    }
                }
            }
        }
    }
}
