/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
****************************************************************************/

import QtQuick 2.1
import HelperWidgets 2.0
import QtQuick.Layouts 1.0
import QtQuick.Controls 1.0 as Controls

Controls.ComboBox {
    id: comboBox

    property variant backendValue
    property color textColor: colorLogic.textColor

    onTextColorChanged: setColor()

    editable: true
    model: ["Arial", "Times New Roman", "Courier", "Verdana", "Tahoma"]

    onModelChanged: {
        editText = backendValue.valueToString
    }

    style: CustomComboBoxStyle {
        textColor: comboBox.textColor
    }

    ColorLogic {
        id: colorLogic
        backendValue: comboBox.backendValue
        property string textValue: backendValue.value
        onTextValueChanged: {
            comboBox.editText = textValue
        }
    }

    Layout.fillWidth: true

    onCurrentTextChanged: {
        if (backendValue === undefined)
            return;

        if (backendValue.value !== currentText)
            backendValue.value = currentText;
    }

    ExtendedFunctionButton {
        x: 2
        y: 4
        backendValue: comboBox.backendValue
        visible: comboBox.enabled
    }

    Component.onCompleted: {
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
