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

import StudioControls as SC

Item {
    width: DialogValues.stylesPaneWidth

    Component.onCompleted: {
        dialogBox.stylesLoaded = true;

        /*
         * TODO: roleNames is called before the backend model (in the proxy class StyleModel) is
         * loaded, which may be buggy. But I found no way to force refresh the model, so as to
         * reload the role names afterwards. setting styleModel.dynamicRoles doesn't appear to do
         * anything.
        */
    }

    Component.onDestruction: {
        dialogBox.stylesLoaded = false;
    }

    Rectangle {
        color: DialogValues.lightPaneColor
        anchors.fill: parent

        Item {
            x: DialogValues.stylesPanePadding                               // left padding
            width: parent.width - DialogValues.stylesPanePadding * 2        // right padding
            height: parent.height

            ColumnLayout {
                anchors.fill: parent
                spacing: 5

                Text {
                    id: styleTitleText
                    text: qsTr("Style")
                    Layout.minimumHeight: DialogValues.dialogTitleTextHeight
                    font.weight: Font.DemiBold
                    font.pixelSize: DialogValues.paneTitlePixelSize
                    lineHeight: DialogValues.paneTitleLineHeight
                    lineHeightMode: Text.FixedHeight
                    color: DialogValues.textColor
                    verticalAlignment: Qt.AlignVCenter

                    function refresh() {
                        text = qsTr("Style") + " (" + styleModel.rowCount() + ")"
                    }
                }

                SC.ComboBox {   // Style Filter ComboBox
                    actionIndicatorVisible: false
                    currentIndex: 0
                    textRole: "text"
                    valueRole: "value"
                    font.pixelSize: DialogValues.defaultPixelSize

                    model: ListModel {
                        ListElement { text: qsTr("All"); value: "all" }
                        ListElement { text: qsTr("Light"); value: "light" }
                        ListElement { text: qsTr("Dark"); value: "dark" }
                    }

                    implicitWidth: parent.width

                    onActivated: (index) => {
                        styleModel.filter(currentValue.toLowerCase());
                        styleTitleText.refresh();
                    }
                } // Style Filter ComboBox

                ListView {
                    id: stylesList
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: styleModel

                    focus: true
                    boundsBehavior: Flickable.StopAtBounds

                    highlightFollowsCurrentItem: false

                    onCurrentIndexChanged: {
                        if (styleModel.rowCount() > 0)
                            dialogBox.styleIndex = stylesList.currentIndex;
                    }

                    delegate: ItemDelegate {
                        id: delegateId
                        height: styleImage.height + DialogValues.styleImageBorderWidth + styleText.height + 1
                        width: stylesList.width

                        Rectangle {
                            anchors.fill: parent
                            color: DialogValues.lightPaneColor

                            Column {
                                spacing: 0
                                anchors.fill: parent

                                Rectangle {
                                    border.color: index == stylesList.currentIndex ? DialogValues.textColorInteraction : "transparent"
                                    border.width: index == stylesList.currentIndex ? DialogValues.styleImageBorderWidth : 0
                                    color: "transparent"
                                    width: parent.width
                                    height: parent.height - styleText.height

                                    Image {
                                        id: styleImage
                                        asynchronous: false
                                        source: "image://newprojectdialog_library/" + styleModel.iconId(model.index)
                                        width: 200
                                        height: 262
                                        x: DialogValues.styleImageBorderWidth
                                        y: DialogValues.styleImageBorderWidth
                                    }
                                } // Rectangle

                                Text {
                                    id: styleText
                                    text: model.display
                                    font.pixelSize: DialogValues.defaultPixelSize
                                    lineHeight: DialogValues.defaultLineHeight
                                    height: 18
                                    lineHeightMode: Text.FixedHeight
                                    horizontalAlignment: Text.AlignHCenter
                                    width: parent.width
                                    color: DialogValues.textColor
                                }
                            } // Column
                        } // Rectangle

                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                stylesList.currentIndex = index
                            }
                        }
                    }

                    Connections {
                        target: styleModel
                        function onModelReset() {
                            stylesList.currentIndex = dialogBox.styleIndex;
                            stylesList.currentIndexChanged();
                            styleTitleText.refresh();
                        }
                    }
                } // ListView
            } // ColumnLayout
        } // Parent Item
    } // Rectangle
}
