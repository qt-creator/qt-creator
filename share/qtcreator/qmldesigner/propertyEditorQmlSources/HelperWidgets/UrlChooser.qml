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

        ExtendedFunctionButton {
            x: 2
            anchors.verticalCenter: parent.verticalCenter
            backendValue: urlChooser.backendValue
            visible: urlChooser.enabled
        }

        property bool isComplete: false

        function setCurrentText(text) {
            if (text === "")
                return


            var index = comboBox.find(textValue)
            if (index === -1)
                currentIndex = -1
            editText = textValue
        }

        property string textValue: backendValue.valueToString
        onTextValueChanged: {
            setCurrentText(textValue)
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

            setCurrentText(textValue)
        }
        onAccepted: {
            if (!comboBox.isComplete)
                return;

            if (backendValue.value !== currentText)
                backendValue.value = currentText;
        }

        onActivated: {
            var cText = textAt(index)
            print(cText)
            if (backendValue === undefined)
                return;

            if (!comboBox.isComplete)
                return;

            if (backendValue.value !== cText)
                backendValue.value = cText;
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
            setCurrentText(textValue)
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
                if (fileModel.fileName != "")
                    backendValue.value = fileModel.fileName
                darkPanel.opacity = 0
            }
        }
    }
}
