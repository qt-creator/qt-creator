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
import QtQuick.Controls 1.1 as Controls
import QtQuick.Layouts 1.0
import QtQuick.Controls.Styles 1.1

RowLayout {
    id: urlChooser
    property variant backendValue

    property color textColor: colorLogic.highlight ? colorLogic.textColor : "white"

    property string filter: "*.png *.gif *.jpg *.bmp *.jpeg *.svg"


    FileResourcesModel {
        modelNodeBackendProperty: modelNodeBackend
        filter: urlChooser.filter
        id: fileModel
    }

    ColorLogic {
        id: colorLogic
        backendValue: urlChooser.backendValue
    }

    Controls.ComboBox {
        id: comboBox

        property bool isComplete: false

        property string textValue: backendValue.value
        onTextValueChanged: {
            comboBox.editText = textValue
        }

        Layout.fillWidth: true

        editable: true
        style: CustomComboBoxStyle {
            textColor: urlChooser.textColor
        }

        model: fileModel.fileModel

        onModelChanged: {
            if (!comboBox.isComplete)
                return;

            editText = backendValue.valueToString
        }

        onCurrentTextChanged: {
            if (backendValue === undefined)
                return;

            if (!comboBox.isComplete)
                return;

            if (backendValue.value !== currentText)
                backendValue.value = currentText;
        }

        Component.onCompleted: {
            //Hack to style the text input
            for (var i = 0; i < comboBox.children.length; i++) {
                if (comboBox.children[i].text !== undefined) {
                    comboBox.children[i].color = urlChooser.textColor
                    comboBox.children[i].anchors.rightMargin = 34
                }
            }
            comboBox.isComplete = true
            editText = backendValue.valueToString
        }

    }

    RoundedPanel {
        roundLeft: true
        roundRight: true
        width: 24
        height: 18

        RoundedPanel {
            id: darkPanel
            roundLeft: true
            roundRight: true

            anchors.fill: parent

            opacity: 0

            Behavior on opacity {
                PropertyAnimation {
                    duration: 100
                }
            }


            gradient: Gradient {
                GradientStop {color: '#444' ; position: 0}
                GradientStop {color: '#333' ; position: 1}
            }
        }

        Text {
            renderType: Text.NativeRendering
            text: "..."
            color: urlChooser.textColor
            anchors.centerIn: parent
        }

        MouseArea {
            anchors.fill: parent
            onClicked: {
                darkPanel.opacity = 1
                fileModel.openFileDialog()
                backendValue.value = fileModel.fileName
                darkPanel.opacity = 0
            }
        }
    }
}
