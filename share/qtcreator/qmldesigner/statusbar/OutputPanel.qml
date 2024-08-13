// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import OutputPane
import StudioControls as StudioControls
import StudioTheme as StudioTheme

ScrollView {
    id: root

    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

    function clearOutput() {
        parentListModel.resetModel()
    }

    clip: true

    ScrollBar.vertical: StudioControls.TransientScrollBar {
        id: verticalScrollBar
        style: StudioTheme.Values.viewStyle
        parent: root
        x: root.width - verticalScrollBar.width
        y: 0
        height: root.availableHeight
        orientation: Qt.Vertical

        show: (root.hovered || root.focus)
              && verticalScrollBar.isNeeded
    }

    ColumnLayout {
        id: clayout

        Repeater {
            id: parentList
            model: AppOutputParentModel {
                id: parentListModel
                historyColor: "grey"
                messageColor: "#007b7b"
                errorColor: "#ff6666"
            }

            delegate: ColumnLayout {
                id: parentDelegate
                spacing: 0

                required property int index
                required property string run
                required property color blockColor

                Text {
                    id: timeStampText
                    text: run
                    color: blockColor
                }

                Repeater {
                    id: childList

                    model: AppOutputChildModel {
                        id: childListModel
                        parentModel: parentListModel
                        row: parentDelegate.index
                    }

                    delegate: Column {
                        id: childDelegate

                        required property string message
                        required property color messageColor
                        Text {
                            wrapMode: Text.WordWrap
                            text: message
                            color: messageColor
                            width: root.width - verticalScrollBar.width
                        }
                    }
                }
            }
        }
    }
}
