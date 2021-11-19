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

import QtQuick.Window
import QtQuick.Controls

import QtQuick
import QtQuick.Layouts
import StudioTheme as StudioTheme

GridView {
    id: projectView

    required property Item loader

    header: TabBar {
        id: tabBar
        width: parent.width
        height: DialogValues.projectViewHeaderHeight

        background: Rectangle {
            color: DialogValues.lightPaneColor
        }

        Repeater {
            model: categoryModel

            TabButton {
                padding: 0

                width: headerText.contentWidth + 36

                background: Item { // TabButton background
                    Rectangle { // bottom strip
                        anchors.bottom: parent.bottom
                        anchors.horizontalCenter: parent.horizontalCenter
                        width: headerText.contentWidth
                        height: 6
                        radius: 10
                        color: tabBar.currentIndex === index ? DialogValues.textColorInteraction
                                                             : "transparent"
                    }
                } // TabButton background

                implicitHeight: headerText.height + DialogValues.defaultPadding - 7

                contentItem:  Item {
                    Column {
                        anchors.fill: parent

                        Text {
                            id: headerText
                            color: tabBar.currentIndex == index ? DialogValues.textColorInteraction
                                                                : DialogValues.textColor
                            text: name
                            width: parent.width
                            font.weight: Font.DemiBold
                            font.pixelSize: DialogValues.viewHeaderPixelSize
                            lineHeight: DialogValues.viewHeaderLineHeight
                            lineHeightMode: Text.FixedHeight
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }

                        Item  { width: parent.width; height: 11; }
                    } // Column
                } // Item

                onClicked: {
                    projectModel.setPage(index)
                    projectView.currentIndex = 0
                    projectView.currentIndexChanged()
                }
            } // TabButton
        } // Repeater
    } // Header - TabBar

    cellWidth: DialogValues.projectItemWidth
    cellHeight: DialogValues.projectItemHeight

    boundsBehavior: Flickable.StopAtBounds

    children: [
        Rectangle {
            color: DialogValues.darkPaneColor
            anchors.fill: parent
            z: -1
        }
    ]

    model: projectModel

    // called by onModelReset and when user clicks on an item, or when the header item is changed.
    onCurrentIndexChanged: {
        dialogBox.selectedProject = projectView.currentIndex
        var source = dialogBox.currentProjectQmlPath()
        loader.source = source
    }

    Connections {
        target: projectModel

        // called when data is set (setWizardFactories)
        function onModelReset() {
            currentIndex = 0
            currentIndexChanged()
        }
    }

    delegate: ItemDelegate {
        id: delegate

        width: DialogValues.projectItemWidth
        height: DialogValues.projectItemHeight
        background: null

        function fontIconCode(index) {
            var code = projectModel.fontIconCode(index)
            return code ? code : StudioTheme.Constants.wizardsUnknown
        }

        Column {
            width: parent.width
            height: parent.height

            Label {
                id: projectTypeIcon
                text: fontIconCode(index)
                color: DialogValues.textColor
                width: parent.width
                height: DialogValues.projectItemHeight / 2
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignBottom
                renderType: Text.NativeRendering
                font.pixelSize: 65
                font.family: StudioTheme.Constants.iconFont.family
            }

            Text {
                id: projectTypeLabel
                color: DialogValues.textColor

                text: name
                font.pixelSize: DialogValues.defaultPixelSize
                lineHeight: DialogValues.defaultLineHeight
                lineHeightMode: Text.FixedHeight
                width: parent.width
                height: DialogValues.projectItemHeight / 2
                wrapMode: Text.Wrap
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignTop
            }
        } // Column

        MouseArea {
            anchors.fill: parent
            onClicked: {
                delegate.GridView.view.currentIndex = index
            }
        }

        states: [
            State {
                when: delegate.GridView.isCurrentItem
                PropertyChanges {
                    target: projectTypeLabel
                    color: DialogValues.textColorInteraction
                }

                PropertyChanges {
                    target: projectTypeIcon
                    color: DialogValues.textColorInteraction
                }
            } // State
        ]
    } // ItemDelegate
} // GridView
