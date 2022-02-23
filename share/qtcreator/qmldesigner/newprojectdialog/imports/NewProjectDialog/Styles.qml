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

import QtQuick
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Layouts

import StudioControls as SC
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

            SC.ComboBox { // Style Filter ComboBox
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
                ScrollBar.vertical: SC.VerticalScrollBar {
                    id: styleScrollBar
                    x: stylesList.width + (DialogValues.stylesPanePadding
                                           - StudioTheme.Values.scrollBarThickness) * 0.5
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
                                    border.color: index === stylesList.currentIndex ? DialogValues.textColorInteraction : "transparent"
                                    border.width: index === stylesList.currentIndex ? DialogValues.styleImageBorderWidth : 0
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
