// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0
import QtQuick 6.7
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    height: 41
    color: "#282828"

    signal popupClosed
    signal issuesCheckedSignal
    signal outputCheckedSignal
    signal issuesClearedSignal
    signal outputClearedSignal

    property alias showOutputViewEnabled: showOutputView.enabled
    property alias clearOutputEnabled: clearOutputButton.enabled
    property alias clearIssuesEnabled: clearIssuesButton.enabled

    property alias outputChecked: outputButton.checked
    property alias issuesChecked: issuesButton.checked

    RowLayout {
        id: tabBar

        anchors.verticalCenter: root.verticalCenter
        anchors.left: root.left
        anchors.leftMargin: 15

        TabBarButton {
            id: issuesButton
            autoExclusive: true
            checked: true

            Connections {
                target: issuesButton
                function onClicked() {
                    if (issuesButton.checked) {
                        root.issuesCheckedSignal()
                    } else
                        return
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
                    if (outputButton.checked)   { root.outputCheckedSignal() }
                    else                        { return }
                }
            }
        }
    }

    RowLayout {
        anchors.right: root.right
        anchors.rightMargin: 10

        IconButton {
            id: showOutputView
            imageSource: "images/outputIcon.png"
            idleBack: "#282828"
            hoverBack: "#3a3a3a"
            Connections {
                target: showOutputView
                function onClicked() { root.showOutputViewSignal() }
            }
        }

        IconButton {
            id: clearIssuesButton
            visible: issuesButton.checked
            hoverBack: "#3a3a3a"
            idleBack: "#282828"
            imageSource: "images/thinBin.png"
            Connections {
                target: clearIssuesButton
                function onClicked() {
                    root.issuesClearedSignal()
                }
            }
        }

        IconButton {
            id: clearOutputButton
            visible: outputButton.checked
            hoverBack: "#3a3a3a"
            idleBack: "#282828"
            imageSource: "images/thinBin.png"
            Connections {
                target: clearOutputButton
                function onClicked() { root.outputClearedSignal() }
            }
        }
    }
}
