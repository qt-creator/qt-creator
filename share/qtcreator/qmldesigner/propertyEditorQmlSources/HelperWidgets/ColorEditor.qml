/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/


import QtQuick 2.1
import QtQuick.Layouts 1.0
import QtQuick.Controls 1.0 as Controls


Column {

    width: parent.width - 8

    property alias color: colorButton.color

    property bool supportGradient: false

    property alias caption: label.text

    property variant backendendValue

    property variant value: backendendValue.value

    onValueChanged: {
        color = value
    }

    Timer {
        id: colorEditorTimer
        repeat: false
        interval: 100
        onTriggered: {
            if (backendendValue !== undefined)
                backendendValue.value = colorEditor.color
        }
    }

    id: colorEditor

    onColorChanged: {
        if (!gradientLine.isCompleted)
            return;
        textField.text = gradientLine.colorToString(color);

        if (supportGradient && gradientLine.visible)
            gradientLine.currentColor = color

        if (buttonRow.checkedIndex !== 1)
            //Delay setting the color to keep ui responsive
            colorEditorTimer.restart()
    }

    GradientLine {
        property bool isCompleted: false
        visible: buttonRow.checkedIndex === 1
        id: gradientLine

        width: parent.width

        onCurrentColorChanged: {
            if (supportGradient && gradientLine.visible)
                colorEditor.color = gradientLine.currentColor
        }

        onHasGradientChanged: {
            print("hasGradient")
             if (!supportGradient)
                 return

            if (gradientLine.hasGradient)
                buttonRow.initalChecked = 1
            else
                buttonRow.initalChecked = 0
            buttonRow.checkedIndex = buttonRow.initalChecked
        }

        Component.onCompleted: {
            colorEditor.color = gradientLine.currentColor
            isCompleted= true
        }
    }

    SectionLayout {
        width: parent.width

        rows: 5

        Item {
            height: 0
            width: 2
        }

        Item {
            height: 0
            width: 2
        }

        Label {
            id: label
            text: "Color"
        }

        SecondColumnLayout {

            LineEdit {
                id: textField
                inputMask: "\\#HHHHHHhh"
                showTranslateCheckBox: false

                backendValue: colorEditor.backendendValue

                onAccepted: {
                    colorEditor.color = textField.text
                }
                Layout.fillWidth: true
            }
            ColorCheckButton {
                id: checkButton
                color: backendendValue.value
            }

            ButtonRow {

                id: buttonRow
                exclusive: true

                ButtonRowButton {
                    iconSource: "images/icon_color_solid.png"
                    onClicked: {
                        colorEditor.backendendValue.resetValue()
                        gradientLine.deleteGradient()
                    }
                    tooltip: qsTr("Solid Color")
                }
                ButtonRowButton {
                    visible: supportGradient
                    iconSource: "images/icon_color_gradient.png"
                    onClicked: {
                        colorEditor.backendendValue.resetValue()
                        gradientLine.addGradient()
                    }

                    tooltip: qsTr("Gradient")
                }
                ButtonRowButton {
                    iconSource: "images/icon_color_none.png"
                    onClicked: {
                        colorEditor.color = "#00000000"
                        gradientLine.deleteGradient()
                    }
                    tooltip: qsTr("Transparent")
                }
            }

            ExpandingSpacer {
            }
        }

        ColorButton {
            enabled: buttonRow.checkedIndex !== 2
            opacity: checkButton.checked ? 1 : 0
            id: colorButton
            width: 116
            height: checkButton.checked ? 116 : 0

            Layout.preferredWidth: 116
            Layout.preferredHeight: checkButton.checked ? 116 : 0

            sliderMargins: Math.max(0, label.width - colorButton.width) + 4

        }

        SecondColumnLayout {
        }

        Item {
            height: 4
            width :4
        }

    }
}
