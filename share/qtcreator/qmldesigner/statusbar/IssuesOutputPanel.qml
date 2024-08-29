// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import StudioControls as StudioControls
import StudioTheme as StudioTheme

import ToolBar

StudioControls.PopupDialog {
    id: root
    width: 800

    property Item targetItem

    property alias warningCount: issuesPanel.warningCount
    property alias errorCount: issuesPanel.errorCount

    property alias unreadOutput: outputPanel.unreadMessages

    readonly property bool issuesVisible: issuesPanel.visible && root.visible
    readonly property bool outputVisible: outputPanel.visible && root.visible

    function toggleShowIssuesPanel() {
        if (!root.visible) {
            outputPanel.visible = false
            issuesPanel.visible = true
            root.show(root.targetItem)
        } else {
            if (issuesPanel.visible) {
                root.close()
            } else {
                outputPanel.visible = false
                issuesPanel.visible = true
            }
        }
    }

    function toggleShowOutputPanel() {
        if (!root.visible) {
            issuesPanel.visible = false
            outputPanel.visible = true
            root.show(root.targetItem)
        } else {
            if (outputPanel.visible) {
                root.close()
            } else {
                issuesPanel.visible = false
                outputPanel.visible = true
            }
        }
    }

    onClosing: {
        issuesPanel.visible = false
        outputPanel.visible = false
    }

    titleBar: RowLayout {
        id: toolBar

        anchors.verticalCenter: parent.verticalCenter
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: 0
        anchors.rightMargin: 10

        RowLayout {
            id: leftAlignedButtons
            Layout.alignment: Qt.AlignLeft

            TabBarButton {
                id: issuesButton
                style: StudioTheme.Values.statusbarButtonStyle
                text: qsTr("Issues")
                checked: issuesPanel.visible
                checkedInverted: true

                onClicked: {
                    if (!issuesPanel.visible) {
                        outputPanel.visible = false
                        issuesPanel.visible = true
                    }
                }
            }

            TabBarButton {
                id: outputButton
                style: StudioTheme.Values.statusbarButtonStyle
                text: qsTr("Output")
                checked: outputPanel.visible
                checkedInverted: true

                onClicked: {
                    if (!outputPanel.visible) {
                        issuesPanel.visible = false
                        outputPanel.visible = true
                    }
                }
            }
        }

        RowLayout {
            id: rightAlignedButtons
            Layout.alignment: Qt.AlignRight

            StudioControls.IconIndicator {
                id: clearButton
                icon: StudioTheme.Constants.trash_medium
                pixelSize: StudioTheme.Values.myIconFontSize * 1.4
                toolTip: qsTr("Clear")
                onClicked: {
                    if (issuesPanel.visible)
                        issuesPanel.clearIssues()
                    else
                        outputPanel.clearOutput()
                }
            }
        }
    }

    Item {
        id: panels
        width: parent.width
        height: 500

        IssuesPanel {
            id: issuesPanel
            visible: false
            anchors.fill: panels
        }

        OutputPanel {
            id: outputPanel
            visible: false
            anchors.fill: panels
        }
    }
}
