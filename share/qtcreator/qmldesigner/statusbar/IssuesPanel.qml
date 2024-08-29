// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import StudioControls as StudioControls
import StudioTheme as StudioTheme

import OutputPane

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
        show: (issuesPanel.hovered || issuesPanel.focus) && verticalScrollBar.isNeeded
    }

    ColumnLayout {
        Repeater {
            id: listView

            model: MessageModel { id: messageModel }

            delegate: RowLayout {
                spacing: 10

                required property int index
                required property string message
                required property string location
                required property string type

                Text {
                    font.family: StudioTheme.Constants.iconFont.family
                    font.pixelSize: StudioTheme.Values.baseIconFontSize
                    color: (type == "Warning") ? StudioTheme.Values.themeAmberLight
                                               : StudioTheme.Values.themeRedLight
                    text: (type == "Warning") ? StudioTheme.Constants.warning2_medium
                                              : StudioTheme.Constants.error_medium
                }

                Text {
                    text: location
                    color: "#57b9fc"
                    font.pixelSize: 12
                    verticalAlignment: Text.AlignVCenter
                    Layout.preferredHeight: 18
                    font.underline: mouseArea.containsMouse ? true : false

                    MouseArea {
                        id: mouseArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: mouseArea.containsMouse ? Qt.PointingHandCursor : Qt.ArrowCursor
                    }
                }

                Text {
                    color: (type == "Warning") ? StudioTheme.Values.themeAmberLight
                                               : StudioTheme.Values.themeRedLight
                    text: message
                    font.pixelSize: StudioTheme.Values.baseFontSize
                    verticalAlignment: Text.AlignVCenter
                    Layout.preferredHeight: 18
                }
            }
        }
    }
}
