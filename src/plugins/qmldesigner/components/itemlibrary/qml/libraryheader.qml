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

Item {
    id: root
    width: 200
    height: 75

    signal tabChanged(int index)
    signal filterChanged(string filterText)
    signal addModuleClicked()
    signal addAssetClicked()

    function setTab(index)
    {
        tabBar.setCurrentIndex(index);
    }

    function clearSearchFilter()
    {
        searchFilterText.text = "";
    }

    Column {
        anchors.left: parent.left
        anchors.right: parent.right
        spacing: 10

        TabBar {
            id: tabBar
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.leftMargin: 5
            anchors.rightMargin: 5
            spacing: 40

            background: Rectangle {
                color: Theme.color(Theme.QmlDesigner_BackgroundColorDarkAlternate)
            }

            Repeater {
                model: [{title: qsTr("Components"), addToolTip: qsTr("Add Module")},
                        {title: qsTr("Assets"), addToolTip: qsTr("Add new assets to project.")}]

                TabButton {
                    contentItem: Text { // TabButton text
                        text: modelData.title
                        font.pixelSize: 13
                        font.bold: true
                        color: tabBar.currentIndex === index ? "#0094ce" : "#dadada"
                        anchors.bottomMargin: 2
                        horizontalAlignment: Text.AlignLeft
                        verticalAlignment: Text.AlignBottom
                        elide: Text.ElideRight
                    }

                    background: Item { // TabButton background
                        Rectangle { // + button
                            anchors.right: parent.right
                            anchors.bottom: parent.bottom
                            anchors.rightMargin: 2
                            anchors.bottomMargin: 2
                            width: img.width + 10
                            height: img.height + 10
                            color: mouseArea.containsMouse ? "#353535" : "#262626"

                            ToolTip.delay: 500
                            ToolTip.text: modelData.addToolTip
                            ToolTip.visible: mouseArea.containsMouse

                            Image {
                                id: img
                                source: tabBar.currentIndex === index ? "../images/add.png"
                                                                      : "../images/add_unselected.png"
                                anchors.centerIn: parent
                            }

                            MouseArea {
                                id: mouseArea
                                anchors.fill: parent
                                hoverEnabled: true
                                onClicked: index == 0 ? addModuleClicked() : addAssetClicked()
                            }
                        }

                        Rectangle { // bottom strip
                            anchors.bottom: parent.bottom
                            width: parent.width
                            height: 2
                            color: tabBar.currentIndex === index  ? "#0094ce" : "#a8a8a8"
                        }
                    }

                    onClicked: tabChanged(index)
                }
            }
        }

        TextField { // filter
            id: searchFilterText
            placeholderText: qsTr("Search")
            placeholderTextColor: "#a8a8a8"
            color: "#dadada"
            selectedTextColor: "#0094ce"
            background: Rectangle {
                color: "#111111"
                border.color: "#666666"
            }
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.leftMargin: 5
            anchors.rightMargin: 5
            selectByMouse: true

            onTextChanged: filterChanged(text)

            Image { // clear text button
                source: "../images/x.png"
                anchors.right: parent.right
                anchors.rightMargin: 5
                anchors.verticalCenter: parent.verticalCenter
                visible: searchFilterText.text !== ""

                MouseArea {
                    anchors.fill: parent
                    onClicked: searchFilterText.text = ""
                }
            }
        }
    }
}
