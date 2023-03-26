// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick.Window

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import StudioTheme as StudioTheme
import StudioControls as SC

import NewProjectDialog
import BackendApi

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

                        Item { width: DialogValues.dialogLeftPadding; height: 1 } // horizontal spacer

                        Image {
                            asynchronous: false
                            source: "image://newprojectdialog_library/logo"
                            width: DialogValues.logoWidth
                            height: DialogValues.logoHeight
                        }

                        Item { width: 10; height: 1 }

                        Text {
                            text: qsTr("Let's create something wonderful with ")
                            font.pixelSize: DialogValues.dialogTitlePixelSize
                            font.family: "Titillium Web"
                            height: DialogValues.dialogTitleTextHeight
                            lineHeight: DialogValues.dialogTitleLineHeight
                            lineHeightMode: Text.FixedHeight
                            color: DialogValues.textColor
                            verticalAlignment: Text.AlignVCenter
                        }

                        Text {
                            text: qsTr("Qt Design Studio!")
                            font.pixelSize: DialogValues.dialogTitlePixelSize
                            font.family: "Titillium Web"
                            height: DialogValues.dialogTitleTextHeight
                            lineHeight: DialogValues.dialogTitleLineHeight
                            lineHeightMode: Text.FixedHeight
                            color: DialogValues.brandTextColor
                            verticalAlignment: Text.AlignVCenter
                        }
                    } // Row

                    Item { width: 1; height: 11 } // spacer

                    Item {
                        width: parent.width
                        height: DialogValues.paneTitleLineHeight
                        Row {
                            width: parent.width
                            height: DialogValues.paneTitleLineHeight

                            Item { width: DialogValues.dialogLeftPadding; height: 1} // spacer

                            Text {
                                width: parent.width - DialogValues.dialogLeftPadding
                                text: qsTr("Create new project by selecting a suitable Preset and then adjust details.")
                                color: DialogValues.textColor
                                font.pixelSize: DialogValues.paneTitlePixelSize
                                lineHeight: DialogValues.paneTitleLineHeight
                                lineHeightMode: Text.FixedHeight
                            }
                        }
                    }
                    Item { width: parent.width; Layout.fillHeight: true} // spacer
                } // ColumnLayout
            } // Header Item

            Item { // Content Item
                Layout.fillWidth: true
                Layout.fillHeight: true

                RowLayout {
                    x: DialogValues.dialogLeftPadding
                    width: parent.width - 70
                    height: parent.height
                    spacing: 0

                    Rectangle { // Left pane
                        color: DialogValues.lightPaneColor
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.minimumWidth: 379 // figured out this number visually
                        Layout.minimumHeight: 261 // figured out this number visually
                        radius: 6

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
                                width: parent.width - 20    // right padding
                                height: DialogValues.presetViewHeaderHeight
                                color: DialogValues.lightPaneColor

                                function selectTab(tabIndex, selectLast = false) {
                                    var item = repeater.itemAt(tabIndex)
                                    tabBarRow.currIndex = tabIndex

                                    presetView.selectLast = selectLast
                                    BackendApi.presetModel.setPage(tabIndex) // NOTE: it resets preset model
                                }

                                Connections {
                                    target: BackendApi

                                    function onUserPresetSaved() {
                                        var customTabIndex = repeater.count - 1
                                        tabBar.selectTab(customTabIndex, true)
                                    }

                                    function onLastUserPresetRemoved() {
                                        tabBar.selectTab(0, false)
                                    }
                                }

                                Row {
                                    id: tabBarRow
                                    spacing: 20
                                    property int currIndex: 0
                                    readonly property string currentTabName:
                                        repeater.count > 0 && repeater.itemAt(currIndex)
                                        ? repeater.itemAt(currIndex).text
                                        : ''

                                    Repeater {
                                        id: repeater
                                        model: BackendApi.categoryModel
                                        Text {
                                            text: name
                                            font.weight: Font.DemiBold
                                            font.pixelSize: DialogValues.viewHeaderPixelSize
                                            verticalAlignment: Text.AlignVCenter

                                            color: {
                                                var color = tabBarRow.currIndex === index
                                                       ? DialogValues.textColorInteraction
                                                       : DialogValues.textColor

                                                return tabItemMouseArea.containsMouse
                                                       ? Qt.darker(color, 1.5)
                                                       : color
                                            }

                                            Behavior on color { ColorAnimation { duration: tabBar.animDur } }

                                            MouseArea {
                                                id: tabItemMouseArea
                                                hoverEnabled: true
                                                anchors.fill: parent
                                                onClicked: {
                                                    tabBar.selectTab(index)
                                                }
                                            }

                                        } // Text
                                    } // Repeater
                                } // tabBarRow

                                Rectangle {
                                    function computeX() {
                                        var item = tabBarRow.children[tabBarRow.currIndex] ?? tabBarRow.children[0]
                                        return item.x;
                                    }

                                    function computeWidth() {
                                        var item = tabBarRow.children[tabBarRow.currIndex] ?? tabBarRow.children[0]
                                        return item.width;
                                    }

                                    id: strip
                                    x: computeX()
                                    width: computeWidth()
                                    height: 5
                                    radius: 2
                                    color: DialogValues.textColorInteraction
                                    anchors.bottom: parent.bottom

                                    Behavior on x { SmoothedAnimation { duration: tabBar.animDur } }
                                    Behavior on width { SmoothedAnimation { duration: strip.width === 0 ? 0 : tabBar.animDur } } // do not animate initial width
                                }
                            } // Rectangle

                            Rectangle {
                                id: presetViewFrame
                                x: 10                       // left padding
                                width: parent.width - 20    // right padding
                                height: DialogValues.presetViewHeight
                                color: DialogValues.darkPaneColor

                                Item {
                                    anchors.fill: parent
                                    anchors.margins: DialogValues.gridMargins

                                    PresetView {
                                        id: presetView
                                        anchors.fill: parent

                                        loader: projectDetailsLoader
                                        currentTabName: tabBarRow.currentTabName

                                        Connections {
                                            target: rootDialog
                                            function onHeightChanged() {
                                                if (rootDialog.height < 720) { // 720 = minimum height big dialog
                                                    DialogValues.presetViewHeight =
                                                            DialogValues.presetItemHeight
                                                            + 2 * DialogValues.gridMargins
                                                } else {
                                                    DialogValues.presetViewHeight =
                                                            DialogValues.presetItemHeight * 2
                                                            + DialogValues.gridSpacing
                                                            + 2 * DialogValues.gridMargins
                                                }
                                            }
                                        }
                                    }
                                }
                            }

                            Item { height: 5; width: parent.width }

                            Text {
                                id: descriptionText
                                text: BackendApi.projectDescription
                                font.pixelSize: DialogValues.defaultPixelSize
                                lineHeight: DialogValues.defaultLineHeight
                                lineHeightMode: Text.FixedHeight
                                leftPadding: 14
                                width: presetViewFrame.width
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
                                    BackendApi.reject();
                                }
                            }

                            Item { Layout.fillWidth: true }

                            SC.AbstractButton {
                                implicitWidth: DialogValues.dialogButtonWidth
                                width: DialogValues.dialogButtonWidth
                                visible: true
                                buttonIcon: qsTr("Create")
                                iconSize: DialogValues.defaultPixelSize
                                enabled: BackendApi.fieldsValid
                                iconFont: StudioTheme.Constants.font

                                onClicked: {
                                    BackendApi.accept();
                                }
                            }
                        } // RowLayout
                    } // Dialog Button Box

                    Item { implicitWidth: DialogValues.dialogLeftPadding - DialogValues.defaultPadding }
                } // RowLayout
            } // Footer
        } // ColumnLayout
    } // Rectangle
}
