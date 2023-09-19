// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Layouts

import StudioControls as StudioControls
import StudioTheme as StudioTheme

import BackendApi

Item {
    width: DialogValues.stylesPaneWidth

    Component.onCompleted: {
        BackendApi.stylesLoaded = true

        /*
         * TODO: roleNames is called before the backend model (in the proxy class StyleModel) is
         * loaded, which may be buggy. But I found no way to force refresh the model, so as to
         * reload the role names afterwards. setting styleModel.dynamicRoles doesn't appear to do
         * anything.
        */
    }

    Component.onDestruction: {
        BackendApi.stylesLoaded = false
    }

    Rectangle {
        color: DialogValues.lightPaneColor
        anchors.fill: parent
        radius: 6

        Item {
            x: DialogValues.stylesPanePadding
            width: parent.width - DialogValues.stylesPanePadding * 2
            height: parent.height

            Text {
                id: styleTitleText
                text: qsTr("Style")
                height: DialogValues.paneTitleTextHeight
                width: parent.width
                font.weight: Font.DemiBold
                font.pixelSize: DialogValues.paneTitlePixelSize
                lineHeight: DialogValues.paneTitleLineHeight
                lineHeightMode: Text.FixedHeight
                color: DialogValues.textColor
                verticalAlignment: Qt.AlignVCenter
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right

                function refresh() {
                    styleTitleText.text = qsTr("Style") + " (" + BackendApi.styleModel.rowCount() + ")"
                }
            }

            StudioControls.ComboBox { // Style Filter ComboBox
                id: styleComboBox
                actionIndicatorVisible: false
                currentIndex: 0
                textRole: "text"
                valueRole: "value"
                font.pixelSize: DialogValues.defaultPixelSize
                width: parent.width

                anchors.top: styleTitleText.bottom
                anchors.topMargin: 5

                model: ListModel {
                    ListElement { text: qsTr("All"); value: "all" }
                    ListElement { text: qsTr("Light"); value: "light" }
                    ListElement { text: qsTr("Dark"); value: "dark" }
                }

                onActivated: (index) => {
                    BackendApi.styleModel.filter(currentValue.toLowerCase())
                    styleTitleText.refresh()
                }
            } // Style Filter ComboBox

            ScrollView {
                id: scrollView

                anchors.top: styleComboBox.bottom
                anchors.topMargin: 11
                anchors.bottom: parent.bottom
                anchors.bottomMargin: DialogValues.stylesPanePadding
                width: parent.width

                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
                ScrollBar.vertical: StudioControls.TransientScrollBar {
                    id: verticalScrollBar
                    style: StudioTheme.Values.viewStyle
                    parent: scrollView
                    x: scrollView.width + (DialogValues.gridMargins - verticalScrollBar.width) * 0.5
                    y: scrollView.topPadding
                    height: scrollView.availableHeight
                    orientation: Qt.Vertical

                    show: (scrollView.hovered || scrollView.focus || verticalScrollBar.inUse)
                          && verticalScrollBar.isNeeded
                }

                ListView {
                    id: stylesList
                    anchors.fill: parent
                    focus: true
                    clip: true
                    model: BackendApi.styleModel
                    boundsBehavior: Flickable.StopAtBounds
                    highlightFollowsCurrentItem: false
                    bottomMargin: -DialogValues.styleListItemBottomMargin

                    onCurrentIndexChanged: {
                        if (BackendApi.styleModel.rowCount() > 0)
                            BackendApi.styleIndex = stylesList.currentIndex
                    }

                    delegate: ItemDelegate {
                        id: delegateId
                        width: stylesList.width
                        height: DialogValues.styleListItemHeight
                        hoverEnabled: true

                        onClicked: stylesList.currentIndex = index

                        background: Rectangle {
                            anchors.fill: parent
                            color: DialogValues.lightPaneColor
                        }

                        contentItem: Item {
                            anchors.fill: parent

                            Column {
                                anchors.fill: parent
                                spacing: DialogValues.styleListItemSpacing

                                Rectangle {
                                    width: DialogValues.styleImageWidth
                                           + 2 * DialogValues.styleImageBorderWidth
                                    height: DialogValues.styleImageHeight
                                            + 2 * DialogValues.styleImageBorderWidth

                                    border.color: {
                                        if (index === stylesList.currentIndex)
                                            return DialogValues.textColorInteraction

                                        if (delegateId.hovered)
                                            return DialogValues.textColor
                                        else
                                            return "transparent"
                                    }

                                    border.width: index === stylesList.currentIndex || delegateId.hovered
                                                  ? DialogValues.styleImageBorderWidth
                                                  : 0

                                    color: "transparent"

                                    Image {
                                        id: styleImage
                                        x: DialogValues.styleImageBorderWidth
                                        y: DialogValues.styleImageBorderWidth
                                        width: DialogValues.styleImageWidth
                                        height: DialogValues.styleImageHeight
                                        asynchronous: false
                                        source: "image://newprojectdialog_library/" + BackendApi.styleModel.iconId(model.index)
                                    }
                                } // Rectangle

                                Text {
                                    id: styleText
                                    text: model.display
                                    font.pixelSize: DialogValues.defaultPixelSize
                                    lineHeight: DialogValues.defaultLineHeight
                                    height: DialogValues.styleTextHeight
                                    lineHeightMode: Text.FixedHeight
                                    horizontalAlignment: Text.AlignHCenter
                                    width: parent.width
                                    color: DialogValues.textColor
                                }
                            } // Column
                        }
                    }

                    Connections {
                        target: BackendApi.styleModel
                        function onModelReset() {
                            stylesList.currentIndex = BackendApi.styleIndex
                            stylesList.currentIndexChanged()
                            styleTitleText.refresh()
                        }
                    }
                } // ListView
            } // ScrollView
        } // Parent Item
    } // Rectangle
}
