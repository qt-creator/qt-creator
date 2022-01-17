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
    id: urlChooser

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
        filter: urlChooser.filter
    }

    ColorLogic {
        id: colorLogic
        backendValue: urlChooser.backendValue
    }

    StudioControls.ComboBox {
        id: comboBox

        property ListModel items: ListModel {}

        implicitWidth: StudioTheme.Values.singleControlColumnWidth
                        + StudioTheme.Values.actionIndicatorWidth
        width: implicitWidth
        // Note: highlightedIndex property isn't used because it has no setter and it doesn't reset
        // when the combobox is closed by focusing on some other control.
        property int hoverIndex: -1

        ToolTip {
            id: toolTip
            visible: comboBox.hover && toolTip.text !== ""
            text: urlChooser.backendValue.valueToString
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
            required property int index

            id: delegateItem
            width: parent.width
            height: StudioTheme.Values.height - 2 * StudioTheme.Values.border
            padding: 0
            highlighted: comboBox.highlightedIndex === index

            indicator: Item {
                id: itemDelegateIconArea
                width: delegateItem.height
                height: delegateItem.height

                Label {
                    id: itemDelegateIcon
                    text: StudioTheme.Constants.tickIcon
                    color: delegateItem.highlighted ? StudioTheme.Values.themeTextSelectedTextColor
                                                    : StudioTheme.Values.themeTextColor
                    font.family: StudioTheme.Constants.iconFont.family
                    font.pixelSize: StudioTheme.Values.spinControlIconSizeMulti
                    visible: comboBox.currentIndex === index ? true : false
                    anchors.fill: parent
                    renderType: Text.NativeRendering
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }

            contentItem: Text {
                leftPadding: itemDelegateIconArea.width
                text: name
                color: delegateItem.highlighted ? StudioTheme.Values.themeTextSelectedTextColor
                                                : StudioTheme.Values.themeTextColor
                font: comboBox.font
                elide: Text.ElideRight
                verticalAlignment: Text.AlignVCenter
            }

            background: Rectangle {
                id: itemDelegateBackground
                x: 0
                y: 0
                width: delegateItem.width
                height: delegateItem.height
                color: delegateItem.highlighted ? StudioTheme.Values.themeInteraction : "transparent"
            }

            ToolTip {
                id: itemToolTip
                visible: delegateItem.hovered && comboBox.highlightedIndex === index
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
            backendValue: urlChooser.backendValue
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
            if (urlChooser.backendValue.isBound)
                return urlChooser.backendValue.expression

            var fullPath = urlChooser.backendValue.valueToString
            return fullPath.substr(fullPath.lastIndexOf('/') + 1)
        }

        onTextValueChanged: comboBox.setCurrentText(comboBox.textValue)

        editable: true
        textRole: "name"
        valueRole: "fullPath"
        model: comboBox.items

        onModelChanged: {
            if (!comboBox.isComplete)
                return

            comboBox.setCurrentText(comboBox.textValue)
        }

        onAccepted: {
            if (!comboBox.isComplete)
                return

            var inputValue = comboBox.editText

            // Check if value set by user matches with a name in the model then pick the full path
            var index = comboBox.find(inputValue)
            if (index !== -1)
                inputValue = comboBox.items.get(index).fullPath

            // Get the currently assigned backend value, extract its file name and compare it to the
            // input value. If they differ the new value needs to be set.
            var currentValue = urlChooser.backendValue.value
            var fileName = currentValue.substr(currentValue.lastIndexOf('/') + 1);

            if (fileName !== inputValue)
                urlChooser.backendValue.value = inputValue

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
            if (urlChooser.backendValue === undefined || !comboBox.isComplete)
                return

            if (index === -1) // select first item if index is invalid
                index = 0

            if (urlChooser.backendValue.value !== comboBox.items.get(index).fullPath)
                urlChooser.backendValue.value = comboBox.items.get(index).fullPath

            comboBox.dirty = false
        }

        Component.onCompleted: {
            // Hack to style the text input
            for (var i = 0; i < comboBox.children.length; i++) {
                if (comboBox.children[i].text !== undefined) {
                    comboBox.children[i].color = urlChooser.textColor
                    comboBox.children[i].anchors.rightMargin = 34
                }
            }
            comboBox.isComplete = true
            comboBox.setCurrentText(comboBox.textValue)
        }
    }

    function createModel() {
        // Build the combobox model
        comboBox.items.clear()

        if (urlChooser.defaultItems !== undefined) {
            for (var i = 0; i < urlChooser.defaultItems.length; ++i) {
                comboBox.items.append({
                    fullPath: urlChooser.defaultItems[i],
                    name: urlChooser.defaultItems[i]
                })
            }
        }

        for (var j = 0; j < fileModel.fullPathModel.length; ++j) {
            comboBox.items.append({
                fullPath: fileModel.fullPathModel[j],
                name: fileModel.fileNameModel[j]
            })
        }
    }

    Connections {
        target: fileModel
        function onFullPathModelChanged() {
            urlChooser.createModel()
            comboBox.setCurrentText(comboBox.textValue)
        }
    }

    onDefaultItemsChanged: urlChooser.createModel()

    Component.onCompleted: urlChooser.createModel()

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
            if (comboBox.popup.opened && !urlChooser.backendValue.isBound) {
                var index = urlChooser.indexOf(comboBox.items,
                                               function(item) {
                                                   return item.fullPath === urlChooser.backendValue.value
                                               })

                if (index !== -1) {
                    comboBox.currentIndex = index
                    comboBox.hoverIndex = index
                    comboBox.editText = comboBox.items.get(index).name
                }
            }
        }
    }

    Spacer { implicitWidth: StudioTheme.Values.twoControlColumnGap }

    IconIndicator {
        icon: StudioTheme.Constants.addFile
        iconColor: urlChooser.textColor
        onClicked: {
            fileModel.openFileDialog()
            if (fileModel.fileName !== "")
                urlChooser.backendValue.value = fileModel.fileName
        }
    }
}
