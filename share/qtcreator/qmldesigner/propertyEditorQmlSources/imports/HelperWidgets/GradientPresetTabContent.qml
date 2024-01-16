// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import HelperWidgets
import StudioControls as StudioControls
import StudioTheme as StudioTheme

Rectangle {
    id: tabBackground
    color: StudioTheme.Values.themeControlBackground
    Layout.fillWidth: true
    Layout.fillHeight: true

    property alias viewModel: gradientTable.model
    property bool editableName: false
    signal presetNameChanged(int id, string name)
    signal deleteButtonClicked(int id)

    property real delegateWidth: 154
    property real delegateHeight: 178
    property real gridCellWidth: 160

    GridView {
        id: gradientTable
        Layout.fillWidth: true
        Layout.fillHeight: true
        anchors.fill: parent
        anchors.leftMargin: 10
        rightMargin: StudioTheme.Values.scrollBarThickness
        clip: true
        delegate: gradientDelegate

        property int gridColumns: (width - rightMargin) / tabBackground.gridCellWidth;
        cellWidth: (width - rightMargin) / gridColumns
        cellHeight: 185

        property bool bothVisible: horizontal.scrollBarVisible && vertical.scrollBarVisible

        ScrollBar.horizontal: HorizontalScrollBar {
            id: horizontal
            parent: gradientTable
        }

        ScrollBar.vertical: VerticalScrollBar {
            id: vertical
            parent: gradientTable
        }

        Component {
            id: gradientDelegate

            Rectangle {
                id: backgroundCard
                color: StudioTheme.Values.themeControlOutline
                clip: true

                property real flexibleWidth: (gradientTable.width - gradientTable.cellWidth * gradientTable.gridColumns) / gradientTable.gridColumns
                property bool isSelected: false

                width: gradientTable.cellWidth + flexibleWidth - 8
                height: tabBackground.delegateHeight
                radius: 16

                function selectPreset(index) {
                    gradientTable.currentIndex = index
                    gradientData.stops = stopsPosList
                    gradientData.colors = stopsColorList
                    gradientData.stopsCount = stopListSize
                    gradientData.presetID = presetID
                    gradientData.presetType = presetTabBar.currentIndex

                    if (gradientData.selectedItem != null)
                        gradientData.selectedItem.isSelected = false

                    backgroundCard.isSelected = true
                    gradientData.selectedItem = backgroundCard
                }

                MouseArea {
                    id: rectMouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: {
                        presetNameBox.edit = false
                        nameInput.focus = false
                        backgroundCard.selectPreset(index)
                    }
                }

                states: [
                    State {
                        name: "default"
                        when: !(rectMouseArea.containsMouse || removeButton.hovered ||
                              (nameMouseArea.containsMouse && !tabBackground.editableName)) &&
                              !backgroundCard.isSelected
                        PropertyChanges {
                            target: backgroundCard
                            color: StudioTheme.Values.themeControlOutline
                            border.width: 0
                        }
                    },
                    State {
                        name: "hovered"
                        when: (rectMouseArea.containsMouse || removeButton.hovered ||
                              (nameMouseArea.containsMouse && !tabBackground.editableName)) &&
                              !backgroundCard.isSelected
                        PropertyChanges {
                            target: backgroundCard
                            color: StudioTheme.Values.themeControlBackgroundHover
                            border.width: StudioTheme.Values.border
                            border.color: StudioTheme.Values.themeInteraction
                        }
                    },
                    State {
                        name: "selected"
                        when: backgroundCard.isSelected
                        PropertyChanges {
                            target: backgroundCard
                            color:StudioTheme.Values.themeInteraction
                            border.width: StudioTheme.Values.border
                            border.color: StudioTheme.Values.themeControlOutlineInteraction
                        }
                    }
                ]

                ColumnLayout {
                    spacing: 2
                    anchors.fill: parent

                    Rectangle {
                        id: gradientRect
                        width: 150
                        height: 150
                        radius: 16
                        Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
                        Layout.topMargin: 2

                        gradient: Gradient {
                            id: showGr
                        }

                        Component {
                            id: stopComponent
                            GradientStop {}
                        }

                        Component.onCompleted: {
                            var stopsAmount = stopListSize
                            var newStops = []
                            for (var i = 0; i < stopsAmount; i++)
                                newStops.push( stopComponent.createObject(showGr, { "position": stopsPosList[i], "color": stopsColorList[i] }) )

                            showGr.stops = newStops
                        }

                        AbstractButton {
                            id: removeButton
                            visible: editableName && (rectMouseArea.containsMouse || removeButton.hovered)
                            backgroundRadius: StudioTheme.Values.smallRectWidth
                            anchors.right: parent.right
                            anchors.rightMargin: 5
                            anchors.top: parent.top
                            anchors.topMargin: 5
                            width: Math.round(StudioTheme.Values.smallRectWidth)
                            height: Math.round(StudioTheme.Values.smallRectWidth)
                            buttonIcon: StudioTheme.Constants.closeCross
                            onClicked: tabBackground.deleteButtonClicked(index)
                        }
                    }

                    Item {
                        id: presetNameBox
                        Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
                        Layout.preferredWidth: backgroundCard.width - 2
                        Layout.preferredHeight: backgroundCard.height - gradientRect.height - 4
                        Layout.bottomMargin: 4

                        property bool edit: false

                        Rectangle {
                            id: nameBackgroundColor
                            enabled: tabBackground.editableName
                            color: "transparent"
                            radius: 16
                            visible: true
                            anchors.fill: parent
                        }

                        ToolTipArea {
                            id: nameMouseArea
                            anchors.fill: parent
                            tooltip: nameText.text
                            propagateComposedEvents: true

                            onClicked: {
                                if (!tabBackground.editableName) {
                                    backgroundCard.selectPreset(index)
                                    return
                                }

                                presetNameBox.edit = true
                                nameInput.forceActiveFocus()
                                // have to select text like this, otherwise there is an issue with long names
                                nameInput.cursorPosition = 0
                                nameInput.cursorPosition = nameInput.length
                                nameInput.selectAll()
                            }
                        }

                        Text {
                            id: nameText
                            text: presetName
                            anchors.fill: parent
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            color: StudioTheme.Values.themeTextColor
                            elide: Text.ElideMiddle
                            maximumLineCount: 1
                        }

                        TextInput {
                            id: nameInput
                            enabled: tabBackground.editableName
                            visible: false
                            text: presetName
                            anchors.fill: parent
                            anchors.leftMargin: 5
                            anchors.rightMargin: 5
                            horizontalAlignment: TextInput.AlignHCenter
                            verticalAlignment: TextInput.AlignVCenter
                            color: StudioTheme.Values.themeTextColor
                            selectionColor: StudioTheme.Values.themeInteraction
                            selectByMouse: true
                            activeFocusOnPress: true
                            wrapMode: TextInput.NoWrap
                            clip: true

                            onEditingFinished: {
                                nameText.text = text
                                tabBackground.presetNameChanged(index, text)
                                presetNameBox.edit = false
                            }

                            Keys.onPressed:function(event) {
                                if (event.key === Qt.Key_Enter || event.key === Qt.Key_Return) {
                                    event.accepted = true
                                    nameInput.editingFinished()
                                    nameInput.focus = false
                                }

                                if (event.key === Qt.Key_Escape) {
                                    event.accepted = true
                                    nameInput.text = nameText.text
                                    nameInput.focus = false
                                }
                            }
                        }

                        states: [
                            State {
                                name: "default"
                                when: tabBackground.editableName && !nameMouseArea.containsMouse && !presetNameBox.edit
                                PropertyChanges {
                                    target: nameBackgroundColor
                                    color: "transparent"
                                    border.width: 0
                                }
                                PropertyChanges { target: nameText; visible: true }
                                PropertyChanges { target: nameInput; visible: false }
                            },
                            State {
                                name: "hovered"
                                when: tabBackground.editableName && nameMouseArea.containsMouse && !presetNameBox.edit
                                PropertyChanges {
                                    target: nameBackgroundColor
                                    color: StudioTheme.Values.themeControlBackgroundPressed
                                    border.width: 0
                                }
                                PropertyChanges { target: nameText; visible: true }
                                PropertyChanges { target: nameInput; visible: false }
                            },
                            State {
                                name: "edit"
                                when: tabBackground.editableName && presetNameBox.edit
                                PropertyChanges {
                                    target: nameBackgroundColor
                                    color: StudioTheme.Values.themeControlBackgroundPressed
                                    border.color: StudioTheme.Values.themeInteraction
                                    border.width: StudioTheme.Values.border
                                }
                                PropertyChanges { target: nameText; visible: false }
                                PropertyChanges { target: nameInput; visible: true }
                            }
                        ]
                    }
                }
            }
        }
    }
}
