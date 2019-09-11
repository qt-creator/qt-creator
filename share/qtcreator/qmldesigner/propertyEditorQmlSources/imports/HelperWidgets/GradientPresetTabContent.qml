/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

import QtQuick 2.11
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.5
import QtQuick.Controls.Styles 1.4
import QtQuick.Dialogs 1.3

import HelperWidgets 2.0
import QtQuickDesignerTheme 1.0
import QtQuick.Controls.Private 1.0 as PrivateControls


Rectangle {
    id: tabBackground
    width: parent.width
    height: parent.height
    color: "#242424"
    anchors.fill: parent

    property alias viewModel : gradientTable.model;
    property bool editableName : false;
    signal presetNameChanged(int id, string name)
    signal deleteButtonClicked(int id)

    property real delegateWidth: 154;
    property real delegateHeight: 178;
    property real gridCellWidth: 160;


    ScrollView {
        Layout.fillWidth: true
        Layout.fillHeight: true
        anchors.fill: parent

        GridView {
            id: gradientTable
            Layout.fillWidth: true
            Layout.fillHeight: true
            anchors.fill: parent
            anchors.leftMargin: 10
            clip: true
            delegate: gradientDelegate

            property int gridColumns: width / tabBackground.gridCellWidth;
            cellWidth: width / gridColumns
            cellHeight: 185

            Component {
                id: gradientDelegate

                Rectangle {
                    id: backgroundCard
                    color: "#404040"
                    clip: true

                    property real flexibleWidth: (gradientTable.width - gradientTable.cellWidth * gradientTable.gridColumns) / gradientTable.gridColumns
                    property bool isSelected: false

                    width: gradientTable.cellWidth + flexibleWidth - 8
                    height: tabBackground.delegateHeight
                    radius: 16

                    MouseArea {
                        id: rectMouseArea
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: {
                            gradientTable.currentIndex = index;
                            gradientData.stops = stopsPosList;
                            gradientData.colors = stopsColorList;
                            gradientData.stopsCount = stopListSize;
                            gradientData.presetID = presetID;
                            gradientData.presetType = presetTabView.currentIndex

                            if (gradientData.selectedItem != null)
                                gradientData.selectedItem.isSelected = false

                            backgroundCard.isSelected = true
                            gradientData.selectedItem = backgroundCard
                        }
                        onEntered: {
                            if (backgroundCard.state != "CLICKED") {
                                backgroundCard.state = "HOVER";
                            }
                        }
                        onExited: {
                            if (backgroundCard.state != "CLICKED") {
                                backgroundCard.state = "USUAL";
                            }
                        }
                    } //mouseArea

                    onIsSelectedChanged: {
                        if (isSelected)
                            backgroundCard.state = "CLICKED"
                        else
                            backgroundCard.state = "USUAL"
                    }

                    states: [
                        State {
                            name: "HOVER"
                            PropertyChanges {
                                target: backgroundCard
                                color: "#606060"
                                border.width: 1
                                border.color: "#029de0"
                            }
                        },
                        State {
                            name: "CLICKED"
                            PropertyChanges {
                                target: backgroundCard
                                color:"#029de0"
                                border.width: 1
                                border.color: "#606060"
                            }
                        },
                        State {
                            name: "USUAL"
                            PropertyChanges
                            {
                                target: backgroundCard
                                color: "#404040"
                                border.width: 0
                            }
                        }
                    ] //states

                    ColumnLayout {
                        spacing: 2
                        anchors.fill: parent

                        Rectangle {
                            width: 150; height: 150
                            id: gradientRect
                            radius: 16
                            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter //was top
                            Layout.topMargin: 2

                            gradient: Gradient {
                                id: showGr
                            }

                            Component {
                                id: stopComponent
                                GradientStop {}
                            }

                            Component.onCompleted: {
                                var stopsAmount = stopListSize;
                                var newStops = [];
                                for (var i = 0; i < stopsAmount; i++ ) {
                                    newStops.push( stopComponent.createObject(showGr, { "position": stopsPosList[i], "color": stopsColorList[i] }) );
                                }
                                showGr.stops = newStops;
                            }

                            Rectangle {
                                id: removeItemRect
                                anchors.right: parent.right
                                anchors.rightMargin: 5
                                anchors.top: parent.top
                                anchors.topMargin: 5
                                height: 16
                                width: 16
                                visible: editableName && rectMouseArea.containsMouse
                                color: "#804682b4"

                                MouseArea {
                                    anchors.fill: parent;
                                    onClicked: tabBackground.deleteButtonClicked(index);
                                }

                                Image {
                                    source: "image://icons/close"
                                    fillMode: Image.PreserveAspectFit
                                    anchors.fill: parent;
                                    Layout.alignment: Qt.AlignCenter
                                } //image remove
                            } //rectangle remove
                        } //rectangle gradient

                        Item {
                            id: presetNameBox
                            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
                            Layout.preferredWidth: backgroundCard.width - 2
                            Layout.preferredHeight: backgroundCard.height - gradientRect.height - 4
                            Layout.bottomMargin: 4

                            property bool readOnly : true;

                            onReadOnlyChanged: {
                                if (!readOnly) {
                                    nameInput.visible = true;
                                    nameInput.forceActiveFocus();

                                    //have to select text like this, otherwise there is an issue with long names
                                    nameInput.cursorPosition = 0;
                                    nameInput.cursorPosition = nameInput.length;
                                    nameInput.selectAll();
                                }
                            } //onReadOnlyChanged

                            Rectangle {
                                id: nameBackgroundColor
                                enabled: tabBackground.editableName
                                color: "transparent"
                                radius: 16
                                visible: true
                                anchors.fill: parent
                            } //rectangle

                            MouseArea {
                                id: nameMouseArea
                                visible: tabBackground.editableName
                                hoverEnabled: true
                                anchors.fill: parent

                                onClicked: presetNameBox.state = "Clicked";

                                onEntered: {
                                    if (presetNameBox.state != "Clicked")
                                    {
                                        presetNameBox.state = "Hover";
                                    }
                                }

                                onExited: {
                                    if (presetNameBox.state != "Clicked")
                                    {
                                        presetNameBox.state = "Normal";
                                    }
                                    PrivateControls.Tooltip.hideText()
                                }

                                Timer {
                                    interval: 1000
                                    running: nameMouseArea.containsMouse && presetNameBox.readOnly
                                    onTriggered: PrivateControls.Tooltip.showText(nameMouseArea, Qt.point(nameMouseArea.mouseX, nameMouseArea.mouseY), presetName)
                                }

                                onCanceled: Tooltip.hideText()

                            } //mouseArea

                            Text {
                                id: nameText
                                visible: presetNameBox.readOnly
                                text: presetName
                                anchors.fill: parent
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                                color: "#ffffff"
                                elide: Text.ElideMiddle
                                maximumLineCount: 1
                            } //text


                            TextInput {
                                id: nameInput
                                visible: !presetNameBox.readOnly
                                text: presetName
                                anchors.fill: parent
                                anchors.leftMargin: 5
                                anchors.rightMargin: 5
                                horizontalAlignment: TextInput.AlignHCenter
                                verticalAlignment: TextInput.AlignVCenter
                                color: "#ffffff"
                                selectionColor: "#029de0"
                                activeFocusOnPress: true
                                wrapMode: TextInput.NoWrap
                                clip: true

                                onEditingFinished: {
                                    nameText.text = text
                                    tabBackground.presetNameChanged(index, text);
                                    presetNameBox.state = "Normal";
                                }

                                Keys.onPressed: {
                                    if (event.key === Qt.Key_Enter || event.key === Qt.Key_Return) {
                                        event.accepted = true;
                                        editingFinished();
                                        focus = false;
                                    } //if
                                } //Keys.onPressed
                            } //textInput

                            states: [
                                State {
                                    name: "Normal"
                                    PropertyChanges {
                                        target: nameBackgroundColor
                                        color: "transparent"
                                        border.width: 0
                                    }
                                    PropertyChanges {
                                        target: presetNameBox
                                        readOnly: true
                                    }
                                },
                                State {
                                    name: "Hover"
                                    PropertyChanges {
                                        target: nameBackgroundColor
                                        color: "#606060"
                                        border.width: 0
                                    }
                                    PropertyChanges {
                                        target: presetNameBox
                                        readOnly: true
                                    }
                                },
                                State {
                                    name: "Clicked"
                                    PropertyChanges {
                                        target: nameBackgroundColor
                                        color: "#606060"
                                        border.color: "#029de0"
                                        border.width: 1
                                    }
                                    PropertyChanges {
                                        target: presetNameBox
                                        readOnly: false
                                    }
                                    PropertyChanges {
                                        target: nameInput
                                        visible: true
                                    }
                                }
                            ] //states
                        } //gradient name Item
                    } //columnLayout
                } //rectangle background
            } //component delegate
        } //gridview
    } //scrollView
} //rectangle
