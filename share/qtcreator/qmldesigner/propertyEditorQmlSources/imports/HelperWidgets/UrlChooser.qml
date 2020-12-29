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
import QtQuickDesignerTheme 1.0
import QtQuick.Layouts 1.0
import QtQuick.Controls 2.5

RowLayout {
    id: urlChooser

    property variant backendValue
    property color textColor: colorLogic.highlight ? colorLogic.textColor : "white"
    property string filter: "*.png *.gif *.jpg *.bmp *.jpeg *.svg *.pbm *.pgm *.ppm *.xbm *.xpm *.hdr *.webp"

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

        // Note: highlightedIndex property isn't used because it has no setter and it doesn't reset
        // when the combobox is closed by focusing on some other control.
        property int hoverIndex: -1

        ToolTip {
            visible: comboBox.hovered
            text: urlChooser.backendValue.valueToString
            delay: 1000
        }

        delegate: ItemDelegate {
            id: delegateItem
            width: parent.width
            height: 20
            highlighted: comboBox.hoverIndex === index

            indicator: Label { // selected item check mark
                padding: 5
                y: (parent.height - height) / 2
                text: StudioTheme.Constants.tickIcon
                font.pixelSize: 10
                font.family: StudioTheme.Constants.iconFont.family
                color: Theme.color(comboBox.hoverIndex === index ? Theme.PanelTextColorLight
                                                                 : Theme.QmlDesigner_HighlightColor)
                visible: comboBox.currentIndex === index
            }

            contentItem: Label {
                leftPadding: 10
                text: modelData
                anchors.top: parent.top
                color: Theme.color(Theme.PanelTextColorLight)
                font.pixelSize: 13
            }

            background: Rectangle {
                anchors.fill: parent
                color: parent.highlighted ? Theme.color(Theme.QmlDesigner_HighlightColor) : "transparent"
            }

            ToolTip {
                visible: delegateItem.hovered && comboBox.highlightedIndex === index
                text: fileModel.fullPathModel[index]
                delay: 1000
            }

            onHoveredChanged: {
                if (hovered)
                    comboBox.hoverIndex = index
            }
        }

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

            var fullPath = urlChooser.backendValue.valueToString;
            var fileName = fullPath.substr(fullPath.lastIndexOf('/') + 1);
            return fileName;
        }

        onTextValueChanged: comboBox.setCurrentText(comboBox.textValue)

        Layout.fillWidth: true

        editable: true

        model: fileModel.fileNameModel

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
            if (urlChooser.backendValue === undefined)
                return

            if (!comboBox.isComplete)
                return

            if (index === -1) // select first item if index is invalid
                index = 0

            if (urlChooser.backendValue.value !== fileModel.fullPathModel[index])
                urlChooser.backendValue.value = fileModel.fullPathModel[index]

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

    Connections {
        target: comboBox
        function onStateChanged(state)
        {
            // update currentIndex when the popup opens to override the default behavior in super classes
            // that selects currentIndex based on values in the combo box.
            if (comboBox.popup.opened) {
                var index = fileModel.fullPathModel.indexOf(urlChooser.backendValue.value)
                if (index !== -1) {
                    comboBox.currentIndex = index
                    comboBox.hoverIndex = index
                }
            }
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
