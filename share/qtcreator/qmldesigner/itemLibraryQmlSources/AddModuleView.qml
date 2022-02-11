/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuickDesignerTheme 1.0
import HelperWidgets 2.0
import StudioTheme 1.0 as StudioTheme

Column {
    id: root

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
                model: addModuleModel

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
                        onClicked: rootView.handleAddImport(index)
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

