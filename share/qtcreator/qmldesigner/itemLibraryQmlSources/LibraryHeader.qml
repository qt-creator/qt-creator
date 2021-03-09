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
import StudioControls 1.0 as StudioControls
import StudioTheme 1.0 as StudioTheme

Item {
    id: root
    width: 200
    height: 75

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
                    topPadding: 4
                    bottomPadding: 4
                    contentItem: Item {
                        implicitHeight: plusButton.height

                        Text { // TabButton text
                            text: modelData.title
                            font.pixelSize: 13
                            font.bold: true
                            color: tabBar.currentIndex === index ? "#0094ce" : "#dadada"
                            anchors.left: parent.left
                            anchors.top: parent.top
                            anchors.bottom: parent.bottom
                            anchors.right: plusButton.left
                            anchors.bottomMargin: 2
                            horizontalAlignment: Text.AlignLeft
                            verticalAlignment: Text.AlignBottom
                            elide: Text.ElideRight
                        }

                        Rectangle { // + button
                            id: plusButton
                            anchors.right: parent.right
                            anchors.top: parent.top
                            anchors.topMargin: 1
                            width: 24
                            height: 24
                            color: mouseArea.containsMouse ? "#353535" : "#262626"

                            ToolTip.delay: 500
                            ToolTip.text: modelData.addToolTip
                            ToolTip.visible: mouseArea.containsMouse

                            Label { // + sign
                                text: StudioTheme.Constants.plus
                                font.family: StudioTheme.Constants.iconFont.family
                                font.pixelSize: StudioTheme.Values.myIconFontSize
                                verticalAlignment: Text.AlignVCenter
                                horizontalAlignment: Text.AlignHCenter
                                anchors.centerIn: parent
                                color: tabBar.currentIndex === index  ? "#0094ce" : "#a8a8a8"
                            }

                            MouseArea {
                                id: mouseArea
                                anchors.fill: parent
                                hoverEnabled: true
                                onClicked: index == 0 ? rootView.handleAddModule()
                                                      : rootView.handleAddAsset()
                            }
                        }
                    }

                    background: Item { // TabButton background
                        Rectangle { // bottom strip
                            anchors.bottom: parent.bottom
                            width: parent.width
                            height: 2
                            color: tabBar.currentIndex === index  ? "#0094ce" : "#a8a8a8"
                        }
                    }

                    onClicked: rootView.handleTabChanged(index);
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

            onTextChanged: rootView.handleSearchfilterChanged(text)

            Rectangle { // x button
                width: 15
                height: 15
                anchors.right: parent.right
                anchors.rightMargin: 5
                anchors.verticalCenter: parent.verticalCenter
                visible: searchFilterText.text !== ""
                color: xMouseArea.containsMouse ? "#353535" : "transparent"

                Label {
                    text: StudioTheme.Constants.closeCross
                    font.family: StudioTheme.Constants.iconFont.family
                    font.pixelSize: StudioTheme.Values.myIconFontSize
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignHCenter
                    anchors.centerIn: parent
                    color: "#dadada"
                }

                MouseArea {
                    id: xMouseArea
                    hoverEnabled: true
                    anchors.fill: parent
                    onClicked: searchFilterText.text = ""
                }
            }
        }
    }
}
