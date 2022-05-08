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

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import HelperWidgets 2.0
import StudioControls 1.0 as StudioControls
import StudioTheme 1.0 as StudioTheme
import QtQuickDesignerTheme 1.0

Row {
    id: root

    property variant backendValue
    property color textColor: colorLogic.highlight ? colorLogic.textColor
                                                   : StudioTheme.Values.themeTextColor
    property string filter: "*.png *.gif *.jpg *.bmp *.jpeg *.svg *.pbm *.pgm *.ppm *.xbm *.xpm *.hdr *.ktx *.webp"

    // This property takes an array of strings which define default items that should be added
    // to the ComboBox model in addition to the items from the FileResourcesModel. This is used
    // by QtQuick3D to add built-in primitives to the model.
    property var defaultItems

    FileResourcesModel {
        id: fileModel
        modelNodeBackendProperty: modelNodeBackend
        filter: root.filter
    }

    ColorLogic {
        id: colorLogic
        backendValue: root.backendValue
    }

    StudioControls.FilterComboBox {
        id: comboBox

        property ListModel listModel: ListModel {}

        implicitWidth: StudioTheme.Values.singleControlColumnWidth
                        + StudioTheme.Values.actionIndicatorWidth
        width: implicitWidth
        allowUserInput: true

        // Note: highlightedIndex property isn't used because it has no setter and it doesn't reset
        // when the combobox is closed by focusing on some other control.
        property int hoverIndex: -1

        ToolTip {
            id: toolTip
            visible: comboBox.hover && toolTip.text !== ""
            text: root.backendValue.valueToString
            delay: StudioTheme.Values.toolTipDelay
            height: StudioTheme.Values.toolTipHeight
            background: Rectangle {
                color: StudioTheme.Values.themeToolTipBackground
                border.color: StudioTheme.Values.themeToolTipOutline
                border.width: StudioTheme.Values.border
            }
            contentItem: Text {
                color: StudioTheme.Values.themeToolTipText
                text: toolTip.text
                verticalAlignment: Text.AlignVCenter
            }
        }

        delegate: ItemDelegate {
            required property string fullPath
            required property string name
            required property int group
            required property int index

            id: delegateRoot
            width: comboBox.popup.width - comboBox.popup.leftPadding - comboBox.popup.rightPadding
                   - (comboBox.popupScrollBar.visible ? comboBox.popupScrollBar.contentItem.implicitWidth + 2
                                                      : 0) // TODO Magic number
            height: StudioTheme.Values.height - 2 * StudioTheme.Values.border
            padding: 0
            hoverEnabled: true
            highlighted: comboBox.highlightedIndex === delegateRoot.DelegateModel.itemsIndex

            onHoveredChanged: {
                if (delegateRoot.hovered && !comboBox.popupMouseArea.active)
                    comboBox.setHighlightedIndexItems(delegateRoot.DelegateModel.itemsIndex)
            }

            onClicked: comboBox.selectItem(delegateRoot.DelegateModel.itemsIndex)

            indicator: Item {
                id: itemDelegateIconArea
                width: delegateRoot.height
                height: delegateRoot.height

                Label {
                    id: itemDelegateIcon
                    text: StudioTheme.Constants.tickIcon
                    color: delegateRoot.highlighted ? StudioTheme.Values.themeTextSelectedTextColor
                                                    : StudioTheme.Values.themeTextColor
                    font.family: StudioTheme.Constants.iconFont.family
                    font.pixelSize: StudioTheme.Values.spinControlIconSizeMulti
                    visible: comboBox.currentIndex === delegateRoot.DelegateModel.itemsIndex ? true
                                                                                             : false
                    anchors.fill: parent
                    renderType: Text.NativeRendering
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }

            contentItem: Text {
                leftPadding: itemDelegateIconArea.width
                text: name
                color: delegateRoot.highlighted ? StudioTheme.Values.themeTextSelectedTextColor
                                                : StudioTheme.Values.themeTextColor
                font: comboBox.font
                elide: Text.ElideRight
                verticalAlignment: Text.AlignVCenter
            }

            background: Rectangle {
                x: 0
                y: 0
                width: delegateRoot.width
                height: delegateRoot.height
                color: delegateRoot.highlighted ? StudioTheme.Values.themeInteraction
                                                : "transparent"
            }

            ToolTip {
                id: itemToolTip
                visible: delegateRoot.hovered && comboBox.highlightedIndex === index
                text: fullPath
                delay: StudioTheme.Values.toolTipDelay
                height: StudioTheme.Values.toolTipHeight
                background: Rectangle {
                    color: StudioTheme.Values.themeToolTipBackground
                    border.color: StudioTheme.Values.themeToolTipOutline
                    border.width: StudioTheme.Values.border
                }
                contentItem: Text {
                    color: StudioTheme.Values.themeToolTipText
                    text: itemToolTip.text
                    verticalAlignment: Text.AlignVCenter
                }
            }
        }

        actionIndicator.icon.color: extFuncLogic.color
        actionIndicator.icon.text: extFuncLogic.glyph
        actionIndicator.onClicked: extFuncLogic.show()
        actionIndicator.forceVisible: extFuncLogic.menuVisible

        ExtendedFunctionLogic {
            id: extFuncLogic
            backendValue: root.backendValue
            onReseted: comboBox.editText = ""
        }

        property bool isComplete: false
        property bool dirty: false

        onEditTextChanged: comboBox.dirty = true

        function setCurrentText(text) {
            var index = comboBox.find(text)
            if (index === -1)
                comboBox.currentIndex = -1

            comboBox.editText = text
            comboBox.dirty = false
        }

        // Takes into account applied bindings
        property string textValue: {
            if (root.backendValue.isBound)
                return root.backendValue.expression

            var fullPath = root.backendValue.valueToString
            return fullPath.substr(fullPath.lastIndexOf('/') + 1)
        }

        onTextValueChanged: comboBox.setCurrentText(comboBox.textValue)

        onModelChanged: {
            if (!comboBox.isComplete)
                return

            comboBox.setCurrentText(comboBox.textValue)
        }

        onAccepted: {
            if (!comboBox.isComplete)
                return

            let inputValue = comboBox.editText

            // Check if value set by user matches with a name in the model then pick the full path
            let index = comboBox.find(inputValue)
            if (index !== -1)
                inputValue = comboBox.items.get(index).model.fullPath

            root.backendValue.value = inputValue

            comboBox.dirty = false
        }

        onFocusChanged: {
            if (comboBox.dirty)
               comboBox.handleActivate(comboBox.currentIndex)
        }

        onCompressedActivated: function (index) {
            comboBox.handleActivate(index)
        }

        function handleActivate(index) {
            if (root.backendValue === undefined || !comboBox.isComplete)
                return

            let inputValue = comboBox.editText

            if (index >= 0)
                inputValue = comboBox.items.get(index).model.fullPath

            if (root.backendValue.value !== inputValue)
                root.backendValue.value = inputValue

            comboBox.dirty = false
        }

        Component.onCompleted: {
            // Hack to style the text input
            for (var i = 0; i < comboBox.children.length; i++) {
                if (comboBox.children[i].text !== undefined) {
                    comboBox.children[i].color = root.textColor
                    comboBox.children[i].anchors.rightMargin = 34
                }
            }
            comboBox.isComplete = true
            comboBox.setCurrentText(comboBox.textValue)
        }
    }

    function createModel() {
        // Build the combobox model
        comboBox.listModel.clear()
        // While adding items to the model this binding needs to be interrupted, otherwise the
        // update function of the SortFilterModel is triggered every time on append() which makes
        // QtDS very slow. This will happen when selecting different items in the scene.
        comboBox.model = {}

        if (root.defaultItems !== undefined) {
            for (var i = 0; i < root.defaultItems.length; ++i) {
                comboBox.listModel.append({
                    fullPath: root.defaultItems[i],
                    name: root.defaultItems[i],
                    group: 0
                })
            }
        }

        for (var j = 0; j < fileModel.fullPathModel.length; ++j) {
            comboBox.listModel.append({
                fullPath: fileModel.fullPathModel[j],
                name: fileModel.fileNameModel[j],
                group: 1
            })
        }

        comboBox.model = Qt.binding(function() { return comboBox.listModel })
    }

    Connections {
        target: fileModel
        function onFullPathModelChanged() {
            root.createModel()
            comboBox.setCurrentText(comboBox.textValue)
        }
    }

    onDefaultItemsChanged: root.createModel()

    Component.onCompleted: root.createModel()

    function indexOf(model, criteria) {
        for (var i = 0; i < model.count; ++i) {
            if (criteria(model.get(i)))
                return i
        }
        return -1
    }

    Connections {
        target: comboBox
        function onStateChanged(state) {
            // update currentIndex when the popup opens to override the default behavior in super classes
            // that selects currentIndex based on values in the combo box.
            if (comboBox.popup.opened && !root.backendValue.isBound) {
                var index = root.indexOf(comboBox.items,
                                               function(item) {
                                                   return item.fullPath === root.backendValue.value
                                               })

                if (index !== -1) {
                    comboBox.currentIndex = index
                    comboBox.hoverIndex = index
                    comboBox.editText = comboBox.items.get(index).model.name
                }
            }
        }
    }

    Spacer { implicitWidth: StudioTheme.Values.twoControlColumnGap }

    IconIndicator {
        icon: StudioTheme.Constants.addFile
        iconColor: root.textColor
        onClicked: {
            fileModel.openFileDialog()
            if (fileModel.fileName !== "")
                root.backendValue.value = fileModel.fileName
        }
    }
}
