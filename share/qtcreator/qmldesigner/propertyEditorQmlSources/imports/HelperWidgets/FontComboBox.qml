/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

import QtQuick 2.1
import HelperWidgets 2.0
import QtQuick.Layouts 1.0
import StudioControls 1.0 as StudioControls

StudioControls.ComboBox {
    id: comboBox

    property variant backendValue
    property color textColor: colorLogic.textColor

    labelColor: colorLogic.textColor

    onTextColorChanged: setColor()

    editable: true

    property string fontFilter: "*.ttf *.otf"


    FileResourcesModel {
        modelNodeBackendProperty: modelNodeBackend
        filter: comboBox.fontFilter
        id: fileModel
    }

    function fontUrlToName(url) {
        var fontLoader = Qt.createQmlObject('import QtQuick 2.0; FontLoader { source: \"' + url + '\"; }',
                                           comboBox,
                                           "dynamicFontLoader");
        return fontLoader.name
    }

    function setupModel() {
        var files = fileModel.fileModel
        var familyNames = ["Arial", "Times New Roman", "Courier", "Verdana", "Tahoma"]

        files.forEach(function (item, index) {
            var name = fontUrlToName(fileModel.dirPath + "/" + item)
            familyNames.push(name)
        });

        familyNames.sort()
        comboBox.model = familyNames
    }

    onModelChanged: {
        editText = comboBox.backendValue.valueToString
    }

    ExtendedFunctionLogic {
        id: extFuncLogic
        backendValue: comboBox.backendValue
    }

    actionIndicator.icon.color: extFuncLogic.color
    actionIndicator.icon.text: extFuncLogic.glyph
    actionIndicator.onClicked: extFuncLogic.show()
    actionIndicator.forceVisible: extFuncLogic.menuVisible

    property bool showExtendedFunctionButton: true

    actionIndicator.visible: showExtendedFunctionButton

    ColorLogic {
        id: colorLogic
        backendValue: comboBox.backendValue
        property string textValue: comboBox.backendValue.valueToString
        onTextValueChanged: {
            comboBox.editText = textValue
        }
    }

    Layout.fillWidth: true

    onAccepted: {
        if (backendValue === undefined)
            return;

        if (editText === "")
            return

        if (backendValue.value !== editText)
            backendValue.value = editText;
    }

    onActivated: {
        if (backendValue === undefined)
            return;

        if (editText === "")
            return

        var indexText = comboBox.textAt(index)

        if (backendValue.value !== indexText)
            backendValue.value = indexText;
    }

    Connections {
        target: modelNodeBackend
        onSelectionChanged: {
            comboBox.editText = backendValue.value
            setupModel()
        }
    }

    Component.onCompleted: {
        setupModel()
        //Hack to style the text input
        for (var i = 0; i < comboBox.children.length; i++) {
            if (comboBox.children[i].text !== undefined) {
                comboBox.children[i].color = comboBox.textColor
                comboBox.children[i].anchors.rightMargin = 34
                comboBox.children[i].anchors.leftMargin = 18
            }
        }
    }
    function setColor() {
        //Hack to style the text input
        for (var i = 0; i < comboBox.children.length; i++) {
            if (comboBox.children[i].text !== undefined) {
                comboBox.children[i].color = comboBox.textColor
            }
        }
    }

}
