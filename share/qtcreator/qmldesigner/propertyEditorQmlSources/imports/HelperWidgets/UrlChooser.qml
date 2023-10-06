// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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

    // Current item
    property string absoluteFilePath: ""

    property alias comboBox: comboBox
    property alias spacer: spacer
    property alias actionIndicatorVisible: comboBox.actionIndicatorVisible

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

        hasActiveDrag: activeDragSuffix !== "" && root.filter.includes(activeDragSuffix)
        implicitWidth: StudioTheme.Values.singleControlColumnWidth
                        + StudioTheme.Values.actionIndicatorWidth
        width: implicitWidth
        allowUserInput: true

        // Note: highlightedIndex property isn't used because it has no setter and it doesn't reset
        // when the combobox is closed by focusing on some other control.
        property int hoverIndex: -1

        onCurrentIndexChanged: {
            // This is needed to correctly update root.absoluteFilePath in cases where selection
            // changes between two nodes of same type.
            if (currentIndex !== -1 && !root.backendValue.isBound)
                root.absoluteFilePath = fileModel.resolve(root.backendValue.value)
        }

        DropArea {
            id: dropArea

            anchors.fill: parent

            property string assetPath: ""

            onEntered: function(drag) {
                dropArea.assetPath = drag.getDataAsString(drag.keys[0]).split(",")[0]
                drag.accepted = comboBox.hasActiveDrag
                comboBox.hasActiveHoverDrag = drag.accepted
            }

            onExited: comboBox.hasActiveHoverDrag = false

            onDropped: function(drop) {
                drop.accepted = comboBox.hasActiveHoverDrag
                comboBox.editText = dropArea.assetPath
                comboBox.accepted()
                comboBox.hasActiveHoverDrag = false
                root.backendValue.commitDrop(dropArea.assetPath)
            }
        }

        ToolTip {
            id: toolTip
            visible: comboBox.hover && toolTip.text !== ""
            text: root.backendValue?.valueToString ?? ""
            delay: StudioTheme.Values.toolTipDelay

            background: Rectangle {
                color: StudioTheme.Values.themeToolTipBackground
                border.color: StudioTheme.Values.themeToolTipOutline
                border.width: StudioTheme.Values.border
            }

            contentItem: RowLayout {
                spacing: 10

                Item {
                    visible: thumbnail.status === Image.Ready
                    Layout.preferredWidth: 96
                    Layout.preferredHeight: 96

                    Image {
                        id: checker
                        visible: !root.isMesh(root.absoluteFilePath)
                        anchors.fill: parent
                        fillMode: Image.Tile
                        source: "images/checkers.png"
                    }

                    Image {
                        id: thumbnail
                        asynchronous: true
                        height: 96
                        width: 96
                        fillMode: Image.PreserveAspectFit
                        source: {
                            if (root.isBuiltInPrimitive(root.absoluteFilePath))
                                return "image://qmldesigner_thumbnails/"
                                    + root.absoluteFilePath.substring(1, root.absoluteFilePath.length)
                                    + ".builtin"

                            if (fileModel.isLocal(root.absoluteFilePath))
                                return "image://qmldesigner_thumbnails/" + root.absoluteFilePath

                            return root.absoluteFilePath
                        }
                    }
                }

                ColumnLayout {
                    Text {
                        text: root.fileName(toolTip.text)
                        color: StudioTheme.Values.themeToolTipText
                        font: toolTip.font
                    }

                    Text {
                        Layout.fillWidth: true
                        text: root.isBuiltInPrimitive(toolTip.text) ? qsTr("Built-in primitive")
                                                                    : toolTip.text
                        font: toolTip.font
                        color: StudioTheme.Values.themeToolTipText
                        wrapMode: Text.WordWrap
                    }
                }
            }
        }

        delegate: ItemDelegate {
            required property string absoluteFilePath
            required property string relativeFilePath
            required property string name
            required property int group
            required property int index

            id: delegateRoot
            width: comboBox.popup.width - comboBox.popup.leftPadding - comboBox.popup.rightPadding
            height: StudioTheme.Values.height - 2 * StudioTheme.Values.border
            padding: 0
            hoverEnabled: true
            highlighted: comboBox.highlightedIndex === delegateRoot.DelegateModel.itemsIndex

            onHoveredChanged: {
                if (delegateRoot.hovered && !comboBox.popupMouseArea.active)
                    comboBox.setHighlightedIndexItems(delegateRoot.DelegateModel.itemsIndex)
            }

            onClicked: comboBox.selectItem(delegateRoot.DelegateModel.itemsIndex)

            contentItem: Text {
                leftPadding: 8
                text: name
                color: {
                    if (!delegateRoot.enabled)
                        return comboBox.style.text.disabled

                    if (comboBox.currentIndex === delegateRoot.DelegateModel.itemsIndex)
                        return comboBox.style.text.selectedText

                    return comboBox.style.text.idle
                }
                font: comboBox.font
                elide: Text.ElideRight
                verticalAlignment: Text.AlignVCenter
            }

            background: Rectangle {
                x: 0
                y: 0
                width: delegateRoot.width
                height: delegateRoot.height
                color: {
                    if (!delegateRoot.enabled)
                        return "transparent"

                    if (delegateRoot.hovered && comboBox.currentIndex === delegateRoot.DelegateModel.itemsIndex)
                        return comboBox.style.interactionHover

                    if (comboBox.currentIndex === delegateRoot.DelegateModel.itemsIndex)
                        return comboBox.style.interaction

                    if (delegateRoot.hovered)
                        return comboBox.style.background.hover

                    return "transparent"
                }
            }

            ToolTip {
                id: delegateToolTip
                visible: delegateRoot.hovered
                text: delegateRoot.relativeFilePath
                delay: StudioTheme.Values.toolTipDelay

                background: Rectangle {
                    color: StudioTheme.Values.themeToolTipBackground
                    border.color: StudioTheme.Values.themeToolTipOutline
                    border.width: StudioTheme.Values.border
                }

                contentItem: RowLayout {
                    spacing: 10

                    Item {
                        visible: delegateThumbnail.status === Image.Ready
                        Layout.preferredWidth: 96
                        Layout.preferredHeight: 96

                        Image {
                            id: delegateChecker
                            visible: !root.isMesh(delegateRoot.absoluteFilePath)
                            anchors.fill: parent
                            fillMode: Image.Tile
                            source: "images/checkers.png"
                        }

                        Image {
                            id: delegateThumbnail
                            asynchronous: true
                            sourceSize.height: 96
                            sourceSize.width: 96
                            height: 96
                            width: 96
                            fillMode: Image.PreserveAspectFit
                            source: {
                                if (root.isBuiltInPrimitive(delegateRoot.name))
                                    return "image://qmldesigner_thumbnails/"
                                        + delegateRoot.name.substring(1, delegateRoot.name.length)
                                        + ".builtin"

                                return "image://qmldesigner_thumbnails/" + delegateRoot.absoluteFilePath
                            }
                        }
                    }

                    ColumnLayout {
                        Text {
                            text: delegateRoot.name
                            color: StudioTheme.Values.themeToolTipText
                            font: delegateToolTip.font
                        }

                        Text {
                            Layout.fillWidth: true
                            text: root.isBuiltInPrimitive(delegateToolTip.text)
                                  ? qsTr("Built-in primitive")
                                  : delegateToolTip.text
                            font: delegateToolTip.font
                            color: StudioTheme.Values.themeToolTipText
                            wrapMode: Text.WordWrap
                        }
                    }
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
            comboBox.currentIndex = comboBox.find(text)
            comboBox.setHighlightedIndexItems(comboBox.currentIndex)
            comboBox.autocompleteString = ""
            comboBox.editText = text
            comboBox.dirty = false
        }

        // Takes into account applied bindings
        function updateTextValue() {
            if (root.backendValue.isBound) {
                comboBox.textValue = root.backendValue.expression
            } else {
                // Can be absolute or relative file path
                var filePath = root.backendValue.valueToString
                comboBox.textValue = filePath.substr(filePath.lastIndexOf('/') + 1)
            }

            comboBox.setCurrentText(comboBox.textValue)
        }

        Connections {
            target: root.backendValue

            function onIsBoundChanged() { comboBox.updateTextValue() }
            function onExpressionChanged() { comboBox.updateTextValue() }
            function onValueChangedQml() { comboBox.updateTextValue() }
        }

        property string textValue: ""

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
                inputValue = comboBox.items.get(index).model.relativeFilePath

            root.backendValue.value = inputValue

            if (!root.backendValue.isBound)
                root.absoluteFilePath = fileModel.resolve(root.backendValue.value)

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
                inputValue = comboBox.items.get(index).model.relativeFilePath

            if (root.backendValue.value !== inputValue)
                root.backendValue.value = inputValue

            if (!root.backendValue.isBound)
                root.absoluteFilePath = fileModel.resolve(root.backendValue.value)

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

    function isBuiltInPrimitive(value) {
        return value.startsWith('#')
    }

    function isMesh(value) {
        return root.isBuiltInPrimitive(value)
                || root.hasFileExtension(root.fileName(value), "mesh")
    }

    function hasFileExtension(fileName, extension) {
        return fileName.split('.').pop() === extension
    }

    function fileName(filePath) {
        return filePath.substr(filePath.lastIndexOf('/') + 1)
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
                    absoluteFilePath: "",
                    relativeFilePath: root.defaultItems[i],
                    name: root.defaultItems[i],
                    group: 0
                })
            }
        }

        const myModel = fileModel.model
        for (var j = 0; j < myModel.length; ++j) {
            let item = myModel[j]

            comboBox.listModel.append({
                absoluteFilePath: item.absoluteFilePath,
                relativeFilePath: item.relativeFilePath,
                name: item.fileName,
                group: 1
            })
        }

        comboBox.model = Qt.binding(function() { return comboBox.listModel })
    }

    Connections {
        target: fileModel
        function onModelChanged() {
            root.createModel()
            comboBox.setCurrentText(comboBox.textValue)
        }
    }

    onDefaultItemsChanged: root.createModel()

    Component.onCompleted: {
        root.createModel()
        comboBox.updateTextValue()

        if (!root.backendValue.isBound)
            root.absoluteFilePath = fileModel.resolve(root.backendValue.value)
    }

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
                                                   return item.relativeFilePath === root.backendValue.value
                                               })

                if (index !== -1) {
                    comboBox.currentIndex = index
                    comboBox.hoverIndex = index
                    comboBox.editText = comboBox.items.get(index).model.name
                }
            }
        }
    }

    Spacer { id: spacer
        implicitWidth: StudioTheme.Values.twoControlColumnGap
    }

    IconIndicator {
        icon: StudioTheme.Constants.addFile
        iconColor: root.textColor
        onClicked: {
            fileModel.openFileDialog()
            if (fileModel.fileName !== "") {
                root.backendValue.value = fileModel.fileName
                root.absoluteFilePath = fileModel.resolve(root.backendValue.value)
            }
        }
    }
}
