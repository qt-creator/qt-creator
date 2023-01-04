// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import QtQuick.Layouts 1.15
import StudioControls 1.0 as StudioControls
import StudioTheme 1.0 as StudioTheme

Item {
    id: editableListView

    ExtendedFunctionLogic {
        id: extFuncLogic
        backendValue: editableListView.backendValue
    }

    property var backendValue
    property var model
    onModelChanged: myRepeater.updateModel()

    property alias actionIndicator: actionIndicator
    property alias actionIndicatorVisible: actionIndicator.visible
    property real __actionIndicatorWidth: StudioTheme.Values.squareComponentWidth
    property real __actionIndicatorHeight: StudioTheme.Values.height
    property string typeFilter: "QtQuick3D.Material"
    property int activatedReason: ComboBox.ActivatedReason.Other

    property bool delegateHover: false

    signal add(string value)
    signal remove(int idx)
    signal replace(int idx, string value)

    Layout.preferredWidth: StudioTheme.Values.height * 10
    Layout.preferredHeight: myColumn.height

    Component {
        id: myDelegate

        Row {
            property alias comboBox: itemFilterComboBox
            ListViewComboBox {
                id: itemFilterComboBox

                property int myIndex: index
                property bool empty: itemFilterComboBox.initialModelData === ""

                validator: RegExpValidator { regExp: /(^[a-z_]\w*|^[A-Z]\w*\.{1}([a-z_]\w*\.?)+)/ }

                actionIndicatorVisible: false
                typeFilter: editableListView.typeFilter
                editText: modelData
                initialModelData: modelData
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                width: implicitWidth

                onFocusChanged: {
                    if (itemFilterComboBox.focus)
                        myColumn.currentIndex = index

                    if (itemFilterComboBox.empty && itemFilterComboBox.editText !== "") {
                        myRepeater.dirty = false
                        editableListView.add(itemFilterComboBox.editText)
                    }
                }

                onCompressedActivated: {
                    editableListView.activatedReason = reason

                    if (itemFilterComboBox.empty && itemFilterComboBox.editText !== "") {
                        myRepeater.dirty = false
                        editableListView.add(itemFilterComboBox.editText)
                    } else {
                        editableListView.replace(itemFilterComboBox.myIndex, itemFilterComboBox.editText)
                    }
                }

                onHoverChanged: editableListView.delegateHover = itemFilterComboBox.hover
            }

            Spacer { implicitWidth: StudioTheme.Values.twoControlColumnGap }

            IconIndicator {
                id: closeIndicator
                icon: StudioTheme.Constants.closeCross
                onClicked: {
                    var lastItem = index === myRepeater.localModel.length - 1
                    if (myColumn.currentItem.initialModelData === "") {
                        myRepeater.localModel.pop()
                        myRepeater.dirty = false
                        myRepeater.model = myRepeater.localModel // trigger on change handler
                    } else {
                        editableListView.remove(index)
                    }
                    if (!lastItem)
                        myColumn.currentIndex = index - 1
                }
                onHoveredChanged: editableListView.delegateHover = closeIndicator.hovered
            }
        }
    }

    Row {
        ActionIndicator {
            id: actionIndicator
            icon.visible: editableListView.delegateHover
            icon.color: extFuncLogic.color
            icon.text: extFuncLogic.glyph
            onClicked: extFuncLogic.show()
        }

        Column {
            id: myColumn

            property int currentIndex: -1
            property Item currentItem

            spacing: StudioTheme.Values.sectionRowSpacing

            onCurrentIndexChanged: {
                var tmp = myRepeater.itemAt(myColumn.currentIndex)
                if (tmp !== null)
                    myColumn.currentItem = tmp.comboBox
            }

            Repeater {
                id: myRepeater

                property bool dirty: false
                property var localModel: []

                delegate: myDelegate

                onItemAdded: function(index, item) {
                    if (index === myColumn.currentIndex)
                        myColumn.currentItem = item
                }

                function updateModel() {
                    var lastIndex = myColumn.currentIndex
                    myColumn.currentIndex = -1
                    myRepeater.localModel = []

                    editableListView.model.forEach(function(item) {
                        myRepeater.localModel.push(item)
                    });

                    // if list view is still dirty, then last state had an unfinished/empty ComboBox
                    if (myRepeater.dirty)
                        myRepeater.localModel.push("")

                    myRepeater.model = myRepeater.localModel // trigger on change handler

                    if (lastIndex < 0 && myRepeater.localModel.length > 0)
                        myColumn.currentIndex = 0
                    else if (myRepeater.localModel.length > lastIndex)
                        myColumn.currentIndex = lastIndex
                    else
                        myColumn.currentIndex = myRepeater.localModel.length - 1

                    if (editableListView.activatedReason === ComboBox.ActivatedReason.Other
                        && myColumn.currentItem !== null)
                        myColumn.currentItem.forceActiveFocus()
                }
            }

            ListViewComboBox {
                id: dummyComboBox
                visible: myRepeater.count === 0
                validator: RegExpValidator { regExp: /(^[a-z_]\w*|^[A-Z]\w*\.{1}([a-z_]\w*\.?)+)/ }
                actionIndicatorVisible: false
                typeFilter: editableListView.typeFilter
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                width: implicitWidth

                onFocusChanged: {
                    if (dummyComboBox.editText !== "")
                        editableListView.add(dummyComboBox.editText)
                }

                onCompressedActivated: {
                    editableListView.activatedReason = reason

                    if (dummyComboBox.editText !== "")
                        editableListView.add(dummyComboBox.editText)
                    else
                        editableListView.replace(dummyComboBox.myIndex, dummyComboBox.editText)
                }

                onHoverChanged: editableListView.delegateHover = dummyComboBox.hover
            }

            StudioControls.AbstractButton {
                id: plusButton
                buttonIcon: StudioTheme.Constants.plus
                enabled: !myRepeater.dirty && !(editableListView.backendValue.isInModel
                                                && !editableListView.backendValue.isIdList)
                onClicked: {
                    var idx = myRepeater.localModel.push("") - 1
                    myRepeater.model = myRepeater.localModel // trigger on change handler
                    myRepeater.dirty = true
                    myColumn.currentIndex = idx
                    myColumn.currentItem.forceActiveFocus()
                }

                onHoveredChanged: editableListView.delegateHover = plusButton.hovered
            }
        }
    }
}
