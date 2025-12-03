// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import QtQuick.Layouts 1.15
import HelperWidgets 2.0
import StudioControls 1.0 as StudioControls

StudioControls.ComboBox {
    id: root

    property variant backendValue
    property color textColor: colorLogic.textColor
    property string fontFilter: "*.ttf *.otf"
    property bool showExtendedFunctionButton: true

    hasActiveDrag: activeDragSuffix !== "" && root.fontFilter.includes(activeDragSuffix)
    labelColor: colorLogic.textColor
    editable: true

    onTextColorChanged: root.setColor()

    FontResourcesModel {
        id: fileModel
        modelNodeBackendProperty: modelNodeBackend
    }

    DropArea {
        id: dropArea

        anchors.fill: parent

        property string assetPath: ""

        onEntered: function(drag) {
            dropArea.assetPath = drag.getDataAsString(drag.keys[0]).split(",")[0]
            drag.accepted = root.hasActiveDrag
            root.hasActiveHoverDrag = drag.accepted
        }

        onExited: root.hasActiveHoverDrag = false

        onDropped: function(drop) {
            drop.accepted = root.hasActiveHoverDrag
            var fontLoader = root.createFontLoader("file:" + dropArea.assetPath)
            if (fontLoader.status === FontLoader.Ready) {
                root.backendValue.value = fontLoader.name
                root.currentIndex = root.find(root.backendValue.value)
            }
            root.hasActiveHoverDrag = false
            root.backendValue.commitDrop(dropArea.assetPath)
        }
    }

    function createFontLoader(fontUrl) {
        return Qt.createQmlObject('import QtQuick 2.0; FontLoader { source: "' + fontUrl + '"; }',
                                  root, "dynamicFontLoader")
    }

    function setupModel() {
        root.model = fileModel.model
        root.currentIndex = root.find(root.backendValue.value)
    }

    function setColor() {
        // Hack to style the text input
        for (var i = 0; i < root.children.length; i++) {
            if (root.children[i].text !== undefined) {
                root.children[i].color = root.textColor
            }
        }
    }

    onModelChanged: root.editText = root.backendValue.valueToString

    ExtendedFunctionLogic {
        id: extFuncLogic
        backendValue: root.backendValue
    }

    actionIndicator.icon.color: extFuncLogic.color
    actionIndicator.icon.text: extFuncLogic.glyph
    actionIndicator.onClicked: extFuncLogic.show()
    actionIndicator.forceVisible: extFuncLogic.menuVisible
    actionIndicator.visible: root.showExtendedFunctionButton

    ColorLogic {
        id: colorLogic
        property string textValue: root.backendValue.valueToString
        backendValue: root.backendValue
        onTextValueChanged: root.editText = colorLogic.textValue
    }

    onAccepted: {
        if (root.backendValue === undefined)
            return

        if (root.editText === "")
            return

        if (root.backendValue.value !== root.editText)
            root.backendValue.value = root.editText
    }

    onCompressedActivated: function(index, reason) { root.handleActivate(index) }

    function handleActivate(index)
    {
        if (root.backendValue === undefined)
            return

        if (root.editText === "")
            return

        var indexText = root.textAt(index)

        if (root.backendValue.value !== indexText)
            root.backendValue.value = indexText
    }

    Connections {
        target: modelNodeBackend
        function onSelectionChanged() {
            root.editText = root.backendValue.value
            root.setupModel()
        }
    }

    Component.onCompleted: {
        root.setupModel()
        // Hack to style the text input
        for (var i = 0; i < root.children.length; i++) {
            if (root.children[i].text !== undefined) {
                root.children[i].color = root.textColor
                root.children[i].anchors.rightMargin = 34
                root.children[i].anchors.leftMargin = 18
            }
        }
    }
}
