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

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import StudioTheme as StudioTheme
import StudioControls as SC

import NewProjectDialog

Item {
    id: rootDialog
    width: DialogValues.dialogWidth
    height: DialogValues.dialogHeight

    Rectangle { // the main dialog panel
        anchors.fill: parent
        color: DialogValues.darkPaneColor

        ColumnLayout {
            anchors.fill: parent

            Layout.alignment: Qt.AlignHCenter
            spacing: 0

            Item { width: parent.width; height: 20 } // spacer

            Item { // Header Item
                Layout.fillWidth: true
                implicitHeight: 164

                ColumnLayout {
                    anchors.fill: parent

                    Item { width: parent.width; implicitHeight: 20 } // spacer
                    Row {
                        width: parent.width
                        height: DialogValues.dialogTitleTextHeight
                        Layout.alignment: Qt.AlignHCenter
                        Text {
                            text: qsTr("Welcome to ")
                            font.pixelSize: DialogValues.dialogTitlePixelSize
                            font.family: "Titillium Web"
                            height: DialogValues.dialogTitleTextHeight
                            lineHeight: DialogValues.dialogTitleLineHeight
                            lineHeightMode: Text.FixedHeight
                            color: DialogValues.textColor
                        }

                        Text {
                            text: qsTr("Qt Design Studio")
                            font.pixelSize: DialogValues.dialogTitlePixelSize
                            font.family: "Titillium Web"
                            height: DialogValues.dialogTitleTextHeight
                            lineHeight: DialogValues.dialogTitleLineHeight
                            lineHeightMode: Text.FixedHeight
                            color: DialogValues.textColorInteraction
                        }
                    }

                    Text {
                        width: parent.width
                        text: qsTr("Create new project by selecting a suitable Preset and then adjust details.")
                        color: DialogValues.textColor
                        font.pixelSize: DialogValues.paneTitlePixelSize
                        lineHeight: DialogValues.paneTitleLineHeight
                        lineHeightMode: Text.FixedHeight
                        Layout.alignment: Qt.AlignHCenter
                    }

                    Item { width: parent.width; Layout.fillHeight: true} // spacer
                } // ColumnLayout
            } // Header Item

            Item { // Content Item
                Layout.fillWidth: true
                Layout.fillHeight: true

                RowLayout {
                    x: 35
                    width: parent.width - 70
                    height: parent.height
                    spacing: 0

                    Rectangle { // Left pane
                        color: DialogValues.lightPaneColor
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.minimumWidth: 379 // figured out this number visually
                        Layout.minimumHeight: 261 // figured out this number visually

                        Column {
                            x: DialogValues.defaultPadding                          // left padding
                            width: parent.width - DialogValues.defaultPadding * 2   // right padding
                            height: parent.height

                            Text {
                                text: qsTr("Presets")
                                width: parent.width
                                height: 47
                                font.weight: Font.DemiBold
                                font.pixelSize: DialogValues.paneTitlePixelSize
                                lineHeight: DialogValues.paneTitleLineHeight
                                lineHeightMode: Text.FixedHeight
                                color: DialogValues.textColor
                                verticalAlignment: Qt.AlignVCenter
                            }

                            Rectangle { // TabBar
                                readonly property int animDur: 500
                                id: tabBar
                                x: 10                       // left padding
                                width: parent.width - 64    // right padding
                                height: DialogValues.projectViewHeaderHeight
                                color: DialogValues.lightPaneColor

                                Row {
                                    id: tabBarRow
                                    spacing: 20
                                    property int currIndex: 0

                                    Repeater {
                                        model: categoryModel
                                        Text {
                                            text: name
                                            font.weight: Font.DemiBold
                                            font.pixelSize: DialogValues.viewHeaderPixelSize
                                            verticalAlignment: Text.AlignVCenter
                                            color: tabBarRow.currIndex === index ? DialogValues.textColorInteraction
                                                                                 : DialogValues.textColor
                                            Behavior on color { ColorAnimation { duration: tabBar.animDur } }

                                            MouseArea {
                                                anchors.fill: parent
                                                onClicked: {
                                                    tabBarRow.currIndex = index
                                                    projectModel.setPage(index)
                                                    projectView.currentIndex = 0
                                                    projectView.currentIndexChanged()

                                                    strip.x = parent.x
                                                    strip.width = parent.width
                                                }
                                            }

                                        } // Text
                                    } // Repeater
                                } // tabBarRow

                                Rectangle {
                                    id: strip
                                    width: tabBarRow.children[0].width
                                    height: 5
                                    radius: 2
                                    color: DialogValues.textColorInteraction
                                    anchors.bottom: parent.bottom

                                    Behavior on x { SmoothedAnimation { duration: tabBar.animDur } }
                                    Behavior on width { SmoothedAnimation { duration: strip.width === 0 ? 0 : tabBar.animDur } } // do not animate initial width
                                }

                                Connections {
                                    target: rootDialog
                                    function onWidthChanged() {
                                        if (rootDialog.width < 1200) { // 1200 = the width threshold
                                            tabBar.width = tabBar.parent.width - 20
                                            projectView.width = projectView.parent.width - 20
                                        } else {
                                            tabBar.width = tabBar.parent.width - 64
                                            projectView.width = projectView.parent.width - 64
                                        }
                                    }
                                }
                            } // Rectangle

                            NewProjectView {
                                id: projectView
                                x: 10                       // left padding
                                width: parent.width - 64    // right padding
                                height: DialogValues.projectViewHeight
                                loader: projectDetailsLoader

                                Connections {
                                    target: rootDialog
                                    function onHeightChanged() {
                                        if (rootDialog.height < 700) { // 700 = minimum height big dialog
                                            projectView.height = DialogValues.projectViewHeight / 2
                                        } else {
                                            projectView.height = DialogValues.projectViewHeight
                                        }
                                    }
                                }
                            }

                            Item { height: 5; width: parent.width }

                            Text {
                                id: descriptionText
                                text: dialogBox.projectDescription
                                font.pixelSize: DialogValues.defaultPixelSize
                                lineHeight: DialogValues.defaultLineHeight
                                lineHeightMode: Text.FixedHeight
                                leftPadding: 14
                                width: projectView.width
                                color: DialogValues.textColor
                                wrapMode: Text.WordWrap
                                maximumLineCount: 4
                                elide: Text.ElideRight
                            }
                        }
                    }   // Left pane

                    Loader {
                        id: projectDetailsLoader
                        // we need to specify width because the loaded item needs to use parent sizes
                        width: DialogValues.loadedPanesWidth
                        Layout.fillHeight: true
                        source: ""
                    }
                } // RowLayout
            } //Content Item

            Item { // Footer
                implicitHeight: DialogValues.footerHeight
                implicitWidth: parent.width
                RowLayout {
                    anchors.fill: parent
                    spacing: DialogValues.defaultPadding

                    Item { Layout.fillWidth: true }

                    Item { // Dialog Button Box
                        width: DialogValues.stylesPaneWidth
                        height: parent.height

                        RowLayout {
                            width: DialogValues.stylesPaneWidth
                            implicitWidth: DialogValues.stylesPaneWidth
                            implicitHeight: parent.height

                            SC.AbstractButton {
                                implicitWidth: DialogValues.dialogButtonWidth
                                width: DialogValues.dialogButtonWidth
                                visible: true
                                buttonIcon: qsTr("Cancel")
                                iconSize: DialogValues.defaultPixelSize
                                iconFont: StudioTheme.Constants.font

                                onClicked: {
                                    dialogBox.reject();
                                }
                            }

                            Item { Layout.fillWidth: true }

                            SC.AbstractButton {
                                implicitWidth: DialogValues.dialogButtonWidth
                                width: DialogValues.dialogButtonWidth
                                visible: true
                                buttonIcon: qsTr("Create")
                                iconSize: DialogValues.defaultPixelSize
                                enabled: dialogBox.fieldsValid
                                iconFont: StudioTheme.Constants.font

                                onClicked: {
                                    dialogBox.accept();
                                }
                            }
                        } // RowLayout
                    } // Dialog Button Box

                    Item { implicitWidth: 35 - DialogValues.defaultPadding }
                } // RowLayout
            } // Footer
        } // ColumnLayout
    } // Rectangle
}
