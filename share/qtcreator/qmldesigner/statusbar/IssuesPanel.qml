// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0
import QtQuick 6.7
import QtQuick.Controls
import QtQuick.Layouts
import ToolBar
import OutputPane
import StudioControls as StudioControls
import StudioTheme as StudioTheme

ScrollView {

    id: issuesPanel

    signal showCodeViewSignal

    property int warningCount: messageModel.warningCount
    property int errorCount: messageModel.errorCount

    function clearIssues() { messageModel.resetModel() }

    ScrollBar.vertical: StudioControls.TransientScrollBar {
        id: verticalScrollBar
        style: StudioTheme.Values.viewStyle
        parent: issuesPanel
        x: issuesPanel.width - verticalScrollBar.width
        y: 0
        height: issuesPanel.availableHeight
        orientation: Qt.Vertical

        show: (issuesPanel.hovered || issuesPanel.focus)
              && verticalScrollBar.isNeeded
    }

    ColumnLayout {
        Repeater {
            id: listView

            model: MessageModel {
                id: messageModel
            }

            delegate: RowLayout {
                spacing: 10
                Image {
                    id: typeImage
                    source: {
                        if (type == "Warning")  { "images/warningsActive.png" }
                        else                    { "images/errorActive.png" }
                    }
                    fillMode: Image.PreserveAspectFit
                }

                MyLinkTextButton {
                    id: linkTextWarning
                    linkText: location
                    Layout.preferredHeight: 18

                    Connections {
                        target: linkTextWarning
                        function onClicked() { messageModel.jumpToCode(index) }
                    }
                }

                Text {
                    id: historyTime6
                    color: {
                        if (type == "Warning")  { "#ffbb0c" }
                        else                    { "#ff4848" }
                    }
                    text: qsTr(message)
                    font.pixelSize: 12
                    verticalAlignment: Text.AlignTop
                    Layout.preferredHeight: 18
                }
            }
        }
    }
}
