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

    signal showCodeViewSignal

    property int warningCount: messageModel.warningCount
    property int errorCount: messageModel.errorCount

    function clearIssues() { messageModel.resetModel() }

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

    ColumnLayout {
        Repeater {
            id: listView

            model: MessageModel { id: messageModel }

            delegate: Row {
                id: row

                required property int index
                required property string message
                required property string location
                required property string type

                width: root.width
                spacing: 10

                Text {
                    id: labelIcon
                    font.family: StudioTheme.Constants.iconFont.family
                    font.pixelSize: StudioTheme.Values.baseIconFontSize
                    color: (type == "Warning") ? StudioTheme.Values.themeAmberLight
                                               : StudioTheme.Values.themeRedLight
                    text: (type == "Warning") ? StudioTheme.Constants.warning2_medium
                                              : StudioTheme.Constants.error_medium
                    width: 18
                    height: 18
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                Text {
                    id: labelLocation
                    text: location
                    color: "#57b9fc"
                    font.pixelSize: StudioTheme.Values.baseFontSize
                    verticalAlignment: Text.AlignVCenter
                    font.underline: mouseArea.containsMouse
                    height: 18

                    MouseArea {
                        id: mouseArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: mouseArea.containsMouse ? Qt.PointingHandCursor
                                                             : Qt.ArrowCursor
                    }
                }

                Text {
                    id: labelInfo
                    color: (type == "Warning") ? StudioTheme.Values.themeAmberLight
                                               : StudioTheme.Values.themeRedLight
                    text: message
                    font.pixelSize: StudioTheme.Values.baseFontSize
                    verticalAlignment: Text.AlignTop
                    wrapMode: Text.WordWrap
                    width: row.width - labelIcon.width - labelLocation.width - row.spacing * 2
                }
            }
        }
    }
}
