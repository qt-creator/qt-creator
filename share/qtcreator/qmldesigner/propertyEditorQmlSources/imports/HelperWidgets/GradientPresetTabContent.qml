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

    property int delegateWidth: 153;
    property int delegateHeight: 173;
    property int gridCellWidth: 160;


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
            cellHeight: 180

            Component {
                id: gradientDelegate

                Rectangle {
                    id: backgroundCard
                    color: "#404040"
                    clip: false

                    property real flexibleWidth: (gradientTable.width - gradientTable.cellWidth * gradientTable.gridColumns) / gradientTable.gridColumns
                    property bool isSelected: false

                    width: gradientTable.cellWidth + flexibleWidth - 8; height: tabBackground.delegateHeight
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
                                z: 5
                                clip: true
                                border.width: 1
                                border.color: "#029de0"
                            }
                        },
                        State {
                            name: "CLICKED"
                            PropertyChanges {
                                target: backgroundCard
                                color:"#029de0"
                                z: 4
                                clip: true
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
                                scale: 1.0
                                border.width: 0
                            }
                        }
                    ] //states

                    ColumnLayout {
                        anchors.fill: parent

                        Rectangle {
                            width: 150; height: 150
                            id: gradientRect
                            radius: 16
                            Layout.alignment: Qt.AlignHCenter | Qt.AlignTop
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
                                anchors.rightMargin: 2
                                anchors.top: parent.top
                                anchors.topMargin: 2
                                height: 16
                                width: 16
                                visible: editableName && rectMouseArea.containsMouse
                                color: "#804682b4"


                                MouseArea {
                                    anchors.fill: parent;
                                    onClicked: tabBackground.deleteButtonClicked(index);
                                }

                                Image {
                                    id: remoreItemImg
                                    source: "image://icons/close"
                                    fillMode: Image.PreserveAspectFit
                                    anchors.fill: parent;
                                    Layout.alignment: Qt.AlignCenter
                                }
                            }
                        } //rectangle gradient

                        TextInput {
                            id: presetNameBox
                            readOnly: !editableName
                            text: (presetName)
                            Layout.alignment: Qt.AlignHCenter | Qt.AlignBottom
                            Layout.preferredWidth: backgroundCard.width
                            Layout.topMargin: -5
                            padding: 5.0
                            topPadding: -2.0
                            horizontalAlignment: Text.AlignHCenter
                            wrapMode: Text.Wrap
                            color: "#ffffff"
                            activeFocusOnPress: true

                            onEditingFinished: tabBackground.presetNameChanged(index, text);

                            Keys.onPressed: {
                                if (event.key === Qt.Key_Enter || event.key === Qt.Key_Return) {
                                    event.accepted = true;
                                    editingFinished();
                                    focus = false;
                                }
                            } //Keys.onPressed
                        } //textInput
                    } //columnLayout
                } //rectangle background
            } //component delegate
        } //gridview
    } //scrollView
} //rectangle
