// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import HelperWidgets
import StudioControls as StudioControls
import StudioTheme as StudioTheme

Rectangle {
    id: root

    color: StudioTheme.Values.themePanelBackground

    Column {
        id: col
        padding: 5
        spacing: 5

        Row {
            spacing: 5

            Column {
                spacing: 5

                Text {
                    text: qsTr("Select material:")
                    font.bold: true
                    font.pixelSize: StudioTheme.Values.myFontSize
                    color: StudioTheme.Values.themeTextColor
                }

                ListView {
                    id: materialsListView

                    width: root.width * .5 - 5
                    height: root.height - 60
                    focus: true
                    clip: true
                    boundsBehavior: Flickable.StopAtBounds
                    ScrollBar.vertical: StudioControls.ScrollBar {
                        visible: materialsListView.height < materialsListView.contentHeight
                    }
                    model: materialsModel
                    delegate: Rectangle {
                        width: materialsListView.width
                        height: 20
                        color: ListView.isCurrentItem ? StudioTheme.Values.themeTextSelectionColor
                                                      : "transparent"

                        function id() {
                            return modelData.match(/\((.*)\)/).pop()
                        }

                        Text {
                            text: modelData
                            anchors.verticalCenter: parent.verticalCenter
                            font.pixelSize: StudioTheme.Values.myFontSize
                            color: StudioTheme.Values.themeTextColor
                            leftPadding: 5
                        }

                        MouseArea {
                            anchors.fill: parent

                            onClicked: {
                                materialsListView.currentIndex = index
                                rootView.updatePropsModel(id())
                            }
                        }
                    }
                }
            }

            Column {
                spacing: 5
                Text {
                    text: qsTr("Select property:")
                    font.bold: true
                    font.pixelSize: StudioTheme.Values.myFontSize
                    color: StudioTheme.Values.themeTextColor
                }

                ListView {
                    id: propertiesListView

                    width: root.width * .5 - 5
                    height: root.height - 60
                    focus: true
                    clip: true
                    boundsBehavior: Flickable.StopAtBounds
                    ScrollBar.vertical: StudioControls.ScrollBar {
                        visible: propertiesListView.height < propertiesListView.contentHeight
                    }
                    model: propertiesModel
                    delegate: Rectangle {
                        width: propertiesListView.width
                        height: 20
                        color: ListView.isCurrentItem ? StudioTheme.Values.themeTextSelectionColor
                                                      : "transparent"

                        function propName() {
                            return modelData
                        }

                        Text {
                            text: modelData
                            anchors.verticalCenter: parent.verticalCenter
                            font.pixelSize: StudioTheme.Values.myFontSize
                            color: StudioTheme.Values.themeTextColor
                            leftPadding: 5
                        }

                        MouseArea {
                            anchors.fill: parent

                            onClicked: {
                                propertiesListView.currentIndex = index
                            }
                        }
                    }
                }
            }
        }

        Row {
            spacing: 5
            anchors.right: parent.right
            anchors.rightMargin: 10

            Button {
                text: qsTr("Cancel")

                onClicked: {
                    rootView.closeChooseMatPropsView()
                }
            }

            Button {
                text: qsTr("Apply")

                onClicked: {
                    let matId = materialsListView.currentItem.id()
                    let prop = propertiesListView.currentItem.propName()

                    rootView.applyTextureToProperty(matId, prop)
                }
            }
        }
    }
}
