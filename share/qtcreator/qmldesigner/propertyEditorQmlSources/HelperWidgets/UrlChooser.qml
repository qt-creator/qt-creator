/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of the Qt Quick Controls module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
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
