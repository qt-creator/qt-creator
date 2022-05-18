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
import HelperWidgets 2.0
import StudioControls 1.0 as StudioControls

StudioControls.ComboBox {
    id: root

    property variant backendValue
    property color textColor: colorLogic.textColor
    property string fontFilter: "*.ttf *.otf"
    property bool showExtendedFunctionButton: true

    labelColor: colorLogic.textColor
    editable: true

    onTextColorChanged: root.setColor()

    FileResourcesModel {
        id: fileModel
        modelNodeBackendProperty: modelNodeBackend
        filter: root.fontFilter
    }

    function createFontLoader(fontUrl) {
        return Qt.createQmlObject('import QtQuick 2.0; FontLoader { source: "' + fontUrl + '"; }',
                                  root, "dynamicFontLoader")
    }

    function setupModel() {
        var familyNames = ["Arial", "Times New Roman", "Courier", "Verdana", "Tahoma"] // default fonts

        for (var i = 0; i < fileModel.fullPathModel.length; ++i) { // add custom fonts
            var fontLoader = createFontLoader(fileModel.docPath + "/" + fileModel.fullPathModel[i])
            familyNames.push(fontLoader.name)
        }

        // Remove duplicate family names
        familyNames = [...new Set(familyNames)]
        familyNames.sort()
        root.model = familyNames
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
