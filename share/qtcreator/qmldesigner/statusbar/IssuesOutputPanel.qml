// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0
import QtQuick 6.7
import QtQuick.Controls
import QtQuick.Layouts

import StudioControls as StudioControls

import ToolBar

StudioControls.PopupDialog {

    id: root
    width: 1024

    property Item targetItem

    property alias warningCount: issuesPanel.warningCount
    property alias errorCount: issuesPanel.errorCount

    function toggleShowIssuesPanel() {

        if (!root.visible) {
            issuesButton.checked = true
            outputPanel.visible = false
            issuesPanel.visible = true
            root.show(root.targetItem)
        } else {
            root.visible = false
            root.close()
        }
    }

    function toggleShowOutputPanel() {
        if (!root.visible) {
            outputButton.checked = true
            issuesPanel.visible = false
            outputPanel.visible = true
            root.visible = true
        } else {
            root.visible = false
        }
    }

    titleBar: RowLayout {
        id: toolBar

        anchors.verticalCenter: parent.verticalCenter
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: 15
        anchors.rightMargin: 10

        RowLayout {
            id: leftAlignedButtons
            Layout.alignment: Qt.AlignLeft

            TabBarButton {
                id: issuesButton
                autoExclusive: true
                checked: true

                Connections {
                    target: issuesButton
                    function onClicked() {
                        if (issuesButton.checked) {
                            issuesPanel.visible = true
                            outputPanel.visible = false
                        } else { return }
                    }
                }
            }

            TabBarButton {
                id: outputButton
                labelText: "Output"
                autoExclusive: true

                Connections {
                    target: outputButton
                    function onClicked() {
                        if (outputButton.checked) {
                            issuesPanel.visible = false
                            outputPanel.visible = true
                        } else { return }
                    }
                }
            }
        }
        RowLayout {
            id: rightAlignedButtons
            Layout.alignment: Qt.AlignRight

            // IconButton {
            //     id: showOutputView
            //     imageSource: "images/outputIcon.png"
            //     idleBack: "#282828"
            //     hoverBack: "#3a3a3a"
            //     Connections {
            //         target: showOutputView
            //         function onClicked() {
            //             root.showOutputViewSignal()
            //         }
            //     }
            // }

            IconButtonCheckable {
                id: clearIssuesButton
                visible: issuesButton.checked
                hoverBack: "#3a3a3a"
                idleBack: "#282828"
                imageSource: "images/thinBin.png"
                Connections {
                    target: clearIssuesButton
                    function onClicked() {
                        issuesPanel.clearIssues()
                    }
                }
            }

            IconButtonCheckable {
                id: clearOutputButton
                visible: outputButton.checked
                hoverBack: "#3a3a3a"
                idleBack: "#282828"
                imageSource: "images/thinBin.png"
                Connections {
                    target: clearOutputButton
                    function onClicked() {
                        outputPanel.clearOutput()
                    }
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
