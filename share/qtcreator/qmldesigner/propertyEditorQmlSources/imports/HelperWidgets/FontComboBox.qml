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
    id: comboBox

    property variant backendValue
    property color textColor: colorLogic.textColor
    property string fontFilter: "*.ttf *.otf"
    property bool showExtendedFunctionButton: true

    labelColor: colorLogic.textColor
    editable: true

    onTextColorChanged: setColor()

    FileResourcesModel {
        id: fileModel
        modelNodeBackendProperty: modelNodeBackend
        filter: comboBox.fontFilter
    }

    function createFontLoader(fontUrl) {
        return Qt.createQmlObject('import QtQuick 2.0; FontLoader { source: "' + fontUrl + '"; }',
                                  comboBox, "dynamicFontLoader")
    }

    function setupModel() {
        var familyNames = ["Arial", "Times New Roman", "Courier", "Verdana", "Tahoma"] // default fonts

        for (var i = 0; i < fileModel.fullPathModel.length; ++i) { // add custom fonts
            var fontLoader = createFontLoader(fileModel.docPath + "/" + fileModel.fullPathModel[i])
            familyNames.push(fontLoader.name)
        }

        familyNames.sort()
        comboBox.model = familyNames
    }

    onModelChanged: editText = comboBox.backendValue.valueToString

    ExtendedFunctionLogic {
        id: extFuncLogic
        backendValue: comboBox.backendValue
    }

    actionIndicator.icon.color: extFuncLogic.color
    actionIndicator.icon.text: extFuncLogic.glyph
    actionIndicator.onClicked: extFuncLogic.show()
    actionIndicator.forceVisible: extFuncLogic.menuVisible
    actionIndicator.visible: comboBox.showExtendedFunctionButton

    ColorLogic {
        id: colorLogic
        property string textValue: comboBox.backendValue.valueToString
        backendValue: comboBox.backendValue
        onTextValueChanged: comboBox.editText = colorLogic.textValue
    }

    onAccepted: {
        if (backendValue === undefined)
            return

        if (editText === "")
            return

        if (backendValue.value !== editText)
            backendValue.value = editText;
    }

    onActivated: {
        if (backendValue === undefined)
            return

        if (editText === "")
            return

        var indexText = comboBox.textAt(index)

        if (backendValue.value !== indexText)
            backendValue.value = indexText
    }

    Connections {
        target: modelNodeBackend
        function onSelectionChanged() {
            comboBox.editText = backendValue.value
            setupModel()
        }
    }

    Component.onCompleted: {
        setupModel()
        // Hack to style the text input
        for (var i = 0; i < comboBox.children.length; i++) {
            if (comboBox.children[i].text !== undefined) {
                comboBox.children[i].color = comboBox.textColor
                comboBox.children[i].anchors.rightMargin = 34
                comboBox.children[i].anchors.leftMargin = 18
            }
        }
    }

    function setColor() {
        // Hack to style the text input
        for (var i = 0; i < comboBox.children.length; i++) {
            if (comboBox.children[i].text !== undefined) {
                comboBox.children[i].color = comboBox.textColor
            }
        }
    }

}
