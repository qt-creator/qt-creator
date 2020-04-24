/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

import QtQuick 2.12
import QtQuick.Layouts 1.12
import StudioControls 1.0 as StudioControls
import StudioTheme 1.0 as StudioTheme

Rectangle {
    id: editableListView

    property variant backendValue

    ExtendedFunctionLogic {
        id: extFuncLogic
        backendValue: editableListView.backendValue
    }

    property var model
    onModelChanged: myRepeater.updateModel()

    signal add(string value)
    signal remove(int idx)
    signal replace(int idx, string value)

    property alias actionIndicator: actionIndicator

    property alias actionIndicatorVisible: actionIndicator.visible
    property real __actionIndicatorWidth: StudioTheme.Values.squareComponentWidth
    property real __actionIndicatorHeight: StudioTheme.Values.height

    property string typeFilter: "QtQuick3D.Material"

    color: "transparent"
    border.color: StudioTheme.Values.themeControlOutline
    border.width: StudioTheme.Values.border

    Layout.preferredWidth: StudioTheme.Values.height * 10
    Layout.preferredHeight: myColumn.height

    Component {
        id: myDelegate
        ListViewComboBox {
            id: itemFilterComboBox

            property int myIndex: index
            property bool empty: itemFilterComboBox.initialModelData === ""

            validator: RegExpValidator { regExp: /(^[a-z_]\w*|^[A-Z]\w*\.{1}([a-z_]\w*\.?)+)/ }

            actionIndicatorVisible: false
            typeFilter: editableListView.typeFilter
            editText: modelData
            initialModelData: modelData

            width: editableListView.width

            onFocusChanged: {
                if (itemFilterComboBox.focus) {
                    myColumn.currentIndex = index
                }

                if (itemFilterComboBox.empty && itemFilterComboBox.editText !== "") {
                    myRepeater.dirty = false
                    editableListView.add(itemFilterComboBox.editText)
                }
            }

            onCompressedActivated: {
                if (itemFilterComboBox.empty && itemFilterComboBox.editText !== "") {
                    myRepeater.dirty = false
                    editableListView.add(itemFilterComboBox.editText)
                } else {
                    editableListView.replace(myIndex, itemFilterComboBox.editText)
                }
            }
        }
    }

    Rectangle {
        id: highlightRect
        color: "transparent"
        border.width: StudioTheme.Values.border
        border.color: StudioTheme.Values.themeInteraction
        x: myColumn.currentItem ? myColumn.currentItem.x : 0
        y: myColumn.currentItem ? myColumn.currentItem.y : 0
        z: 10
        width: myColumn.currentItem ? myColumn.currentItem.width : 0
        height: myColumn.currentItem ? myColumn.currentItem.height : 0
    }

    Column {
        id: myColumn

        property int currentIndex: -1
        property Item currentItem

        spacing: -1

        onCurrentIndexChanged: myColumn.currentItem = myRepeater.itemAt(myColumn.currentIndex)

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
            }
        }

        Item {
            id: dummyItem
            visible: myRepeater.count === 0
            width: StudioTheme.Values.height
            height: StudioTheme.Values.height
        }

        Row {
            id: row
            spacing: -StudioTheme.Values.border

            StudioControls.ActionIndicator {
                id: actionIndicator
                width: actionIndicator.visible ? __actionIndicatorWidth : 0
                height: actionIndicator.visible ? __actionIndicatorHeight : 0
                showBackground: true

                icon.color: extFuncLogic.color
                icon.text: extFuncLogic.glyph
                onClicked: extFuncLogic.show()
            }
            StudioControls.AbstractButton {
                buttonIcon: "+"
                iconFont: StudioTheme.Constants.font
                enabled: !myRepeater.dirty && !(editableListView.backendValue.isInModel && !editableListView.backendValue.isIdList)
                onClicked: {
                    var idx = myRepeater.localModel.push("") - 1
                    myRepeater.model = myRepeater.localModel // trigger on change handler
                    myRepeater.dirty = true
                    myColumn.currentIndex = idx
                    myColumn.currentItem.forceActiveFocus()
                }
            }
            StudioControls.AbstractButton {
                buttonIcon: "-"
                iconFont: StudioTheme.Constants.font
                enabled: myRepeater.model.length && !(editableListView.backendValue.isInModel && !editableListView.backendValue.isIdList)
                onClicked: {
                    var lastItem = myColumn.currentIndex === myRepeater.localModel.length - 1
                    if (myColumn.currentItem.initialModelData === "") {
                        myRepeater.localModel.pop()
                        myRepeater.dirty = false
                        myRepeater.model = myRepeater.localModel // trigger on change handler
                    } else {
                        editableListView.remove(myColumn.currentIndex)
                    }
                    if (!lastItem)
                        myColumn.currentIndex = myColumn.currentIndex - 1
                }
            }
            Rectangle {
                color: StudioTheme.Values.themeControlBackground
                border.width: StudioTheme.Values.border
                border.color: StudioTheme.Values.themeControlOutline
                height: StudioTheme.Values.height
                width: editableListView.width - (StudioTheme.Values.height - StudioTheme.Values.border) * (actionIndicatorVisible ? 3 : 2)
            }
        }
    }
}
