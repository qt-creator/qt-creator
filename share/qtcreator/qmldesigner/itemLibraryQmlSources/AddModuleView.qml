// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import HelperWidgets
import StudioTheme as StudioTheme
import ItemLibraryBackend

Column {
    id: root

    property alias adsFocus: listView.adsFocus

    spacing: 5

    signal back()

    Row {
        spacing: 5
        anchors.horizontalCenter: parent.horizontalCenter

        Button {
            anchors.verticalCenter: parent.verticalCenter
            text: "<"
            width: 25
            height: 25
            onClicked: back()
        }

        Text {
            text: qsTr("Select a Module to Add")
            color: StudioTheme.Values.themeTextColor
            font.pixelSize: 16
        }
    }

    ScrollView { // ListView not used because of QTBUG-52941
        id: listView
        width: parent.width
        height: parent.height - y
        clip: true

        Column {
            spacing: 2

            Repeater {
                model: ItemLibraryBackend.addModuleModel

                delegate: Rectangle {
                    id: itemBackground
                    width: listView.width
                    height: isSeparator ? 4 : 25
                    color: StudioTheme.Values.themeListItemBackground
                    visible: importVisible

                    Text {
                        id: itemText
                        text: importUrl
                        color: StudioTheme.Values.themeListItemText
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.leftMargin: 10
                        anchors.rightMargin: 10
                    }

                    MouseArea {
                        id: mouseArea
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: ItemLibraryBackend.rootView.handleAddImport(index)
                        enabled: !isSeparator
                    }

                    states: [
                        State {
                            name: "default"
                            when: !isSeparator && !mouseArea.containsMouse && !mouseArea.pressed
                            PropertyChanges {
                                target: itemBackground
                                color: StudioTheme.Values.themeListItemBackground
                            }
                            PropertyChanges {
                                target: itemText
                                color: StudioTheme.Values.themeListItemText
                            }
                        },
                        State {
                            name: "separator"
                            when: isSeparator
                            PropertyChanges {
                                target: itemBackground
                                color: StudioTheme.Values.themePanelBackground
                            }
                        },
                        State {
                            name: "hover"
                            when: !isSeparator && mouseArea.containsMouse && !mouseArea.containsPress
                            PropertyChanges {
                                target: itemBackground
                                color: StudioTheme.Values.themeListItemBackgroundHover
                            }
                            PropertyChanges {
                                target: itemText
                                color: StudioTheme.Values.themeListItemTextHover
                            }
                        },
                        State {
                            name: "press"
                            when: !isSeparator && mouseArea.containsPress
                            PropertyChanges {
                                target: itemBackground
                                color: StudioTheme.Values.themeListItemBackgroundPress
                            }
                            PropertyChanges {
                                target: itemText
                                color: StudioTheme.Values.themeListItemTextPress
                            }
                        }
                    ]
                }
            }
        }
    }
}

