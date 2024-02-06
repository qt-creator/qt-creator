// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import HelperWidgets
import StudioControls as StudioControls
import StudioTheme as StudioTheme

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

    // These paths will be used for default items if they are defined. Otherwise, default item
    // itself is used as the path.
    property var defaultPaths

    // Current item
    property string absoluteFilePath: ""

    property alias comboBox: comboBox
    property alias spacer: spacer
    property alias actionIndicatorVisible: comboBox.actionIndicatorVisible

    property bool hideDuplicates: true

    FileResourcesModel {
        id: fileModel
        modelNodeBackendProperty: modelNodeBackend
        filter: root.filter
    }

    ColorLogic {
        id: colorLogic
        backendValue: root.backendValue
    }

    component ThumbnailToolTip: ToolTip {
        id: toolTip

        property alias checkerVisible: checker.visible
        property alias thumbnailSource: thumbnail.source

        property alias titleText: title.text
        property alias descriptionText: description.text

        property int maximumWidth: 420

        delay: StudioTheme.Values.toolTipDelay

        background: Rectangle {
            color: StudioTheme.Values.themeToolTipBackground
            border.color: StudioTheme.Values.themeToolTipOutline
            border.width: StudioTheme.Values.border
        }

        contentItem: Row {
            id: row

            readonly property real __epsilon: 2

            height: Math.max(wrapper.visible ? wrapper.height : 0, column.height)
            spacing: 10

            Item {
                id: wrapper
                visible: thumbnail.status === Image.Ready
                width: 96
                height: 96

                Image {
                    id: checker
                    anchors.fill: parent
                    fillMode: Image.Tile
                    source: "images/checkers.png"
                }

                Image {
                    id: thumbnail
                    anchors.fill: parent
                    sourceSize.width: wrapper.width
                    sourceSize.height: wrapper.height
                    asynchronous: true
                    fillMode: Image.PreserveAspectFit
                }
            }

            Column {
                id: column

                property int thumbnailSize: wrapper.visible ? wrapper.width + row.spacing : 0

                spacing: 10
                anchors.verticalCenter: parent.verticalCenter
                width: Math.min(toolTip.maximumWidth - column.thumbnailSize,
                                Math.max(titleTextMetrics.width + row.__epsilon,
                                         descriptionTextMetrics.width + row.__epsilon))

                Text {
                    id: title
                    font: toolTip.font
                    color: StudioTheme.Values.themeToolTipText

                    TextMetrics {
                        id: titleTextMetrics
                        text: title.text
                        font: title.font
                    }
                }

                Text {
                    id: description
                    width: column.width
                    font: toolTip.font
                    color: StudioTheme.Values.themeToolTipText
                    wrapMode: Text.Wrap

                    TextMetrics {
                        id: descriptionTextMetrics
                        text: description.text
                        font: description.font
                    }
                }
            }
        }
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

        ThumbnailToolTip {
            id: rootToolTip

            visible: comboBox.hover && rootToolTip.text !== ""
            text: root.backendValue?.valueToString ?? ""

            checkerVisible: !root.isMesh(root.absoluteFilePath)
            thumbnailSource: {
                if (root.isBuiltInPrimitive(root.absoluteFilePath))
                    return "image://qmldesigner_thumbnails/"
                        + root.absoluteFilePath.substring(1, root.absoluteFilePath.length)
                        + ".builtin"

                if (fileModel.isLocal(root.absoluteFilePath))
                    return "image://qmldesigner_thumbnails/" + root.absoluteFilePath

                return root.absoluteFilePath
            }
            titleText: root.fileName(rootToolTip.text)
            descriptionText: root.isBuiltInPrimitive(rootToolTip.text)
                                ? qsTr("Built-in primitive")
                                : rootToolTip.text
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

            ThumbnailToolTip {
                id: delegateToolTip

                visible: delegateRoot.hovered
                text: delegateRoot.relativeFilePath

                checkerVisible: !root.isMesh(delegateRoot.absoluteFilePath)
                thumbnailSource: {
                    if (root.isBuiltInPrimitive(delegateRoot.name))
                        return "image://qmldesigner_thumbnails/"
                            + delegateRoot.name.substring(1, delegateRoot.name.length)
                            + ".builtin"

                    return "image://qmldesigner_thumbnails/" + delegateRoot.absoluteFilePath
                }
                titleText: delegateRoot.name
                descriptionText: root.isBuiltInPrimitive(delegateToolTip.text)
                                    ? qsTr("Built-in primitive")
                                    : delegateToolTip.text
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

        let nameSet = new Set;

        if (root.defaultItems !== undefined) {
            for (var i = 0; i < root.defaultItems.length; ++i) {
                comboBox.listModel.append({
                    absoluteFilePath: root.defaultPaths ? root.defaultPaths[i]
                                                        : "",
                    relativeFilePath: root.defaultPaths ? root.defaultPaths[i]
                                                        : root.defaultItems[i],
                    name: root.defaultItems[i],
                    group: 0
                })
                nameSet.add(root.defaultItems[i])
            }
        }

        const myModel = fileModel.model
        for (var j = 0; j < myModel.length; ++j) {
            let item = myModel[j]

            if (!root.hideDuplicates || !nameSet.has(item.fileName)) {
                comboBox.listModel.append({
                    absoluteFilePath: item.absoluteFilePath,
                    relativeFilePath: item.relativeFilePath,
                    name: item.fileName,
                    group: 1
                })
                nameSet.add(item.fileName)
            }
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
    onDefaultPathsChanged: root.createModel()
    onHideDuplicatesChanged: root.createModel()

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
