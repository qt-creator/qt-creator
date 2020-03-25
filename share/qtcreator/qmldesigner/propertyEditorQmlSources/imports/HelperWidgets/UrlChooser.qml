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
import StudioControls 1.0 as StudioControls
import StudioTheme 1.0 as StudioTheme
import QtQuick.Layouts 1.0

RowLayout {
    id: urlChooser

    property variant backendValue
    property color textColor: colorLogic.highlight ? colorLogic.textColor : "white"
    property string filter: "*.png *.gif *.jpg *.bmp *.jpeg *.svg *.pbm *.pgm *.ppm *.xbm *.xpm"

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

        actionIndicator.icon.color: extFuncLogic.color
        actionIndicator.icon.text: extFuncLogic.glyph
        actionIndicator.onClicked: extFuncLogic.show()
        actionIndicator.forceVisible: extFuncLogic.menuVisible

        ExtendedFunctionLogic {
            id: extFuncLogic
            backendValue: urlChooser.backendValue
        }

        property bool isComplete: false
        property bool dirty: false

        onEditTextChanged: comboBox.dirty = true

        function setCurrentText(text) {
            var index = comboBox.find(text)
            if (index === -1)
                currentIndex = -1

            comboBox.editText = text
            comboBox.dirty = false
        }

        property string textValue: {
            if (urlChooser.backendValue.isBound)
                return urlChooser.backendValue.expression

            return urlChooser.backendValue.valueToString
        }

        onTextValueChanged: comboBox.setCurrentText(comboBox.textValue)

        Layout.fillWidth: true

        editable: true

        model: fileModel.fileModel

        onModelChanged: {
            if (!comboBox.isComplete)
                return

            comboBox.setCurrentText(comboBox.textValue)
        }

        onAccepted: {
            if (!comboBox.isComplete)
                return

            if (comboBox.backendValue.value !== comboBox.editText)
                comboBox.backendValue.value = comboBox.editText

            comboBox.dirty = false
        }

        onFocusChanged: {
            if (comboBox.dirty)
               comboBox.handleActivate(comboBox.currentIndex)
        }

        onCompressedActivated: comboBox.handleActivate(index)

        function handleActivate(index)
        {
            var cText = comboBox.textAt(index)

            if (index === -1)
                cText = comboBox.editText

            if (urlChooser.backendValue === undefined)
                return

            if (!comboBox.isComplete)
                return

            if (urlChooser.backendValue.value !== cText)
                urlChooser.backendValue.value = cText

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

    StudioControls.AbstractButton {
        buttonIcon: StudioTheme.Constants.addFile
        iconColor: urlChooser.textColor
        onClicked: {
            fileModel.openFileDialog()
            if (fileModel.fileName !== "")
                urlChooser.backendValue.value = fileModel.fileName
        }
    }
}
