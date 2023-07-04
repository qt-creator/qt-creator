// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Layouts
import HelperWidgets 2.0 as HelperWidgets
import StudioControls 1.0 as StudioControls
import StudioTheme 1.0 as StudioTheme

Item {
    id: root

    ExtendedFunctionLogic {
        id: extFuncLogic
        backendValue: root.backendValue
    }

    property var backendValue
    property var model
    onModelChanged: myRepeater.updateModel()

    property alias actionIndicator: actionIndicator
    property alias actionIndicatorVisible: actionIndicator.visible
    property real __actionIndicatorWidth: StudioTheme.Values.squareComponentWidth
    property real __actionIndicatorHeight: StudioTheme.Values.height
    property string typeFilter: "QtQuick3D.Material"
    // This binding is a workaround to overcome the rather long adaption to new Qt versions. This
    // should actually be fixed in the ModelSection.qml by setting the textRole: "idAndName".
    property string textRole: (root.typeFilter === "QtQuick3D.Material") ? "idAndName" : "id"
    property string valueRole: "id"
    property int activatedReason: ComboBox.ActivatedReason.Other

    property bool delegateHover: false
    property bool allowDuplicates: true

    property string extraButtonIcon: "" // setting this will show an extra button
    property string extraButtonToolTip: ""
    signal extraButtonClicked(int idx)

    signal add(string value)
    signal remove(int idx)
    signal replace(int idx, string value)

    Layout.preferredWidth: StudioTheme.Values.height * 10
    Layout.preferredHeight: myColumn.height

    HelperWidgets.ListValidator {
        id: listValidator
        filterList: itemFilterModel.validationItems
    }

    HelperWidgets.ItemFilterModel {
        id: itemFilterModel
        typeFilter: root.typeFilter
        modelNodeBackendProperty: modelNodeBackend
        selectedItems: root.allowDuplicates ? [] : root.model
        validationRoles: [root.textRole, root.valueRole]
    }

    Component {
        id: myDelegate

        Row {
            property alias comboBox: delegateComboBox

            ListViewComboBox {
                id: delegateComboBox

                property int myIndex: index
                property bool empty: delegateComboBox.initialModelData === ""

                validator: listValidator
                actionIndicatorVisible: false
                model: itemFilterModel
                initialModelData: modelData
                textRole: root.textRole
                valueRole: root.valueRole
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                width: delegateComboBox.implicitWidth
                textElidable: true

                onFocusChanged: {
                    if (delegateComboBox.focus) {
                        myColumn.currentIndex = index
                    } else {
                        if (!delegateComboBox.dirty)
                            return

                        // If focus is lost check if text was changed and try to search for it in
                        // the text as well as in the value role.
                        let idx = delegateComboBox.indexOfString(delegateComboBox.editText)
                        if (idx === -1) {
                            delegateComboBox.editText = delegateComboBox.preFocusText
                        } else {
                            delegateComboBox.currentIndex = idx
                            if (delegateComboBox.empty && delegateComboBox.currentValue !== "") {
                                myRepeater.dirty = false
                                root.add(delegateComboBox.currentValue)
                            } else {
                                root.replace(delegateComboBox.myIndex, delegateComboBox.currentValue)
                            }
                        }
                    }
                }

                onCompressedActivated: function(index, reason) {
                    root.activatedReason = reason

                    var curValue = delegateComboBox.availableValue()
                    if (delegateComboBox.empty && curValue) {
                        myRepeater.dirty = false
                        root.add(curValue)
                    } else {
                        root.replace(delegateComboBox.myIndex, curValue)
                    }
                }

                onHoverChanged: root.delegateHover = delegateComboBox.hover
            }

            Spacer { implicitWidth: extraButton.visible ? 5 : StudioTheme.Values.twoControlColumnGap }

            IconIndicator {
                id: extraButton
                icon: root.extraButtonIcon
                tooltip: root.extraButtonToolTip
                onClicked: root.extraButtonClicked(index)
                visible: root.extraButtonIcon !== ""
                enabled: root.model[index] ?? false
            }

            IconIndicator {
                id: closeIndicator
                icon: StudioTheme.Constants.closeCross
                onClicked: {
                    var lastItem = index === myRepeater.localModel.length - 1
                    var tmp = myRepeater.itemAt(index)

                    myColumn.currentIndex = index - 1

                    if (tmp.comboBox.initialModelData === "") {
                        myRepeater.localModel.pop()
                        myRepeater.dirty = false
                        myRepeater.model = myRepeater.localModel // trigger on change handler
                    } else {
                        root.remove(index)
                    }
                }
                onHoveredChanged: root.delegateHover = closeIndicator.hovered
            }
        }
    }

    Row {
        ActionIndicator {
            id: actionIndicator
            icon.visible: root.delegateHover
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

            onCurrentItemChanged: {
                if (myColumn.currentItem !== null)
                    myColumn.currentItem.forceActiveFocus()
            }

            Repeater {
                id: myRepeater

                property bool dirty: false
                property var localModel: []

                delegate: myDelegate

                onItemAdded: function(index, item) {
                    if (index === myColumn.currentIndex)
                        myColumn.currentItem = item.comboBox
                }

                function updateModel() {
                    var lastIndex = myColumn.currentIndex
                    myColumn.currentIndex = -1
                    myRepeater.localModel = []

                    root.model.forEach(function(item) {
                        myRepeater.localModel.push(item)
                    });

                    // if list view is still dirty, then last state had an unfinished/empty ComboBox
                    if (myRepeater.dirty)
                        myRepeater.localModel.push("")

                    myRepeater.model = myRepeater.localModel // trigger on change handler

                    if (myRepeater.localModel.length > lastIndex)
                        myColumn.currentIndex = lastIndex
                    else
                        myColumn.currentIndex = myRepeater.localModel.length - 1

                    if (root.activatedReason === ComboBox.ActivatedReason.Other
                        && myColumn.currentItem !== null)
                        myColumn.currentItem.forceActiveFocus()
                }
            }

            ListViewComboBox {
                id: dummyComboBox
                visible: myRepeater.count === 0
                validator: listValidator
                actionIndicatorVisible: false
                model: itemFilterModel
                textRole: root.textRole
                valueRole: root.valueRole
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                width: dummyComboBox.implicitWidth

                onVisibleChanged: dummyComboBox.currentIndex = -1

                onFocusChanged: {
                    if (dummyComboBox.focus)
                        return

                    if (!dummyComboBox.dirty)
                        return

                    // If focus is lost check if text was changed and try to search for it in
                    // the text as well as in the value role.
                    let idx = dummyComboBox.indexOfString(dummyComboBox.editText)
                    if (idx === -1) {
                        dummyComboBox.editText = dummyComboBox.preFocusText
                    } else {
                        dummyComboBox.currentIndex = idx
                        if (dummyComboBox.currentValue !== "")
                            root.add(dummyComboBox.currentValue)
                    }
                }

                onCompressedActivated: function(index, reason) {
                    root.activatedReason = reason

                    var curValue = dummyComboBox.availableValue()
                    if (curValue !== "")
                        root.add(curValue)
                }

                onHoverChanged: root.delegateHover = dummyComboBox.hover
            }

            StudioControls.AbstractButton {
                id: plusButton
                buttonIcon: StudioTheme.Constants.plus
                enabled: !myRepeater.dirty && !(root.backendValue.isInModel
                                                && !root.backendValue.isIdList)
                onClicked: {
                    var idx = myRepeater.localModel.push("") - 1
                    myRepeater.model = myRepeater.localModel // trigger on change handler
                    myRepeater.dirty = true
                    myColumn.currentIndex = idx
                    myColumn.currentItem.forceActiveFocus()
                }

                onHoveredChanged: root.delegateHover = plusButton.hovered
            }
        }
    }
}
