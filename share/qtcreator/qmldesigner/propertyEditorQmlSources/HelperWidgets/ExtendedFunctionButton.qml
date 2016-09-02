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
import QtQuick.Controls 1.0 as Controls
import QtQuick.Window 2.0
import QtQuick.Controls.Styles 1.1
import "Constants.js" as Constants

Item {
    width: 14
    height: 14

    id: extendedFunctionButton

    property variant backendValue

    property string icon: "image://icons/placeholder"
    property string hoverIcon: "image://icons/submenu"

    property bool active: true


    function setIcon() {
        if (backendValue == null) {
            extendedFunctionButton.icon = "image://icons/placeholder"
        } else if (backendValue.isBound ) {
            if (backendValue.isTranslated) { //translations are a special case
                extendedFunctionButton.icon = "image://icons/placeholder"
            } else {
                extendedFunctionButton.icon = "image://icons/expression"
            }
        } else {
            if (backendValue.complexNode != null && backendValue.complexNode.exists) {
            } else {
                extendedFunctionButton.icon = "image://icons/placeholder"
            }
        }
    }

    onBackendValueChanged: {
        setIcon();
    }

    property bool isBoundBackend: backendValue.isBound;
    property string backendExpression: backendValue.expression;

    onActiveChanged: {
        if (active) {
            setIcon();
            opacity = 1;
        } else {
            opacity = 0;
        }
    }

    onIsBoundBackendChanged: {
        setIcon();
    }

    onBackendExpressionChanged: {
        setIcon();
    }

    Image {
        width: 14
        height: 14
        source: extendedFunctionButton.icon
        anchors.centerIn: parent
    }

    MouseArea {
        hoverEnabled: true
        anchors.fill: parent
        onHoveredChanged: {
            if (containsMouse)
                icon = hoverIcon
            else
                setIcon()
        }
        onClicked: {
            menu.popup();
        }
    }

    Controls.Menu {
        id: menu
        Controls.MenuItem {
            text: "Reset"
            onTriggered: {
                transaction.start();
                backendValue.resetValue();
                backendValue.resetValue();
                transaction.end();
            }
        }
        Controls.MenuItem {
            text: "Set Binding"
            onTriggered: {
                textField.text = backendValue.expression
                expressionDialog.visible = true
                textField.forceActiveFocus()

            }
        }
    }

    Item {

        Rectangle {
            anchors.fill: parent
            color: creatorTheme.QmlDesignerBackgroundColorDarker
            opacity: 0.6
        }

        MouseArea {
            anchors.fill: parent
            onDoubleClicked: expressionDialog.visible = false
        }


        id: expressionDialog
        visible: false
        parent: itemPane

        anchors.fill: parent



        Rectangle {
            x: 4
            onVisibleChanged: {
                var pos  = itemPane.mapFromItem(extendedFunctionButton.parent, 0, 0);
                y = pos.y + 2;
            }

            width: parent.width - 8
            height: 260

            radius: 2
            color: creatorTheme.QmlDesignerBackgroundColorDarkAlternate
            border.color: creatorTheme.QmlDesignerBorderColor

            Label {
                x: 8
                y: 6
                font.bold: true
                text: qsTr("Binding Editor")
            }

            Controls.TextField {

                id: textField

                property bool completionActive: listView.count > 0
                property bool dotCompletion: false
                property int dotCursorPos: 0
                property string prefix

                function commitCompletion() {
                    var cursorPos = textField.cursorPosition

                    var string = textField.text
                    var before = string.slice(0, cursorPos - textField.prefix.length)
                    var after = string.slice(cursorPos)

                    textField.text = before + listView.currentItem.text + after

                    textField.cursorPosition = cursorPos + listView.currentItem.text.length - prefix.length

                    listView.model = null
                }

                ListView {
                    id: listView

                    clip: true
                    cacheBuffer: 0
                    snapMode: ListView.SnapToItem
                    boundsBehavior: Flickable.StopAtBounds
                    visible: textField.completionActive
                    delegate: Text {
                        text: modelData
                        color: creatorTheme.PanelTextColorLight
                        Rectangle {
                            visible: index === listView.currentIndex
                            z: -1
                            anchors.fill: parent
                            color: creatorTheme.QmlDesignerBackgroundColorDarkAlternate
                        }
                    }

                    anchors.top: parent.top
                    anchors.topMargin: 26
                    anchors.bottomMargin: 12
                    anchors.bottom: parent.bottom
                    anchors.left: parent.left
                    anchors.leftMargin: 6
                    width: 200
                    spacing: 2
                }

                verticalAlignment: Text.AlignTop

                anchors.fill: parent
                anchors.leftMargin: 8
                anchors.rightMargin: 8
                anchors.topMargin: 24
                anchors.bottomMargin: 32
                onAccepted: {
                    backendValue.expression = textField.text
                    expressionDialog.visible = false
                }

                Keys.priority: Keys.BeforeItem
                Keys.onPressed: {

                    if (event.key === Qt.Key_Period) {
                        textField.dotCursorPos = textField.cursorPosition + 1
                        var list = autoComplete(textField.text+".", textField.dotCursorPos, false)
                        textField.prefix = list.pop()
                        listView.model = list;
                        textField.dotCompletion = true
                    } else {
                        if (textField.completionActive) {
                            var list2 = autoComplete(textField.text + event.text,
                                                     textField.cursorPosition + event.text.length,
                                                     true)
                            textField.prefix = list2.pop()
                            listView.model = list2;
                        }
                    }
                }

                Keys.onSpacePressed: {
                    if (event.modifiers & Qt.ControlModifier) {
                        var list = autoComplete(textField.text, textField.cursorPosition, true)
                        textField.prefix = list.pop()
                        listView.model = list;
                        textField.dotCompletion = false

                        event.accepted = true;

                        if (list.length == 1)
                            textField.commitCompletion()

                    } else {
                        event.accepted = false
                    }
                }

                Keys.onReturnPressed: {
                    event.accepted = false
                    if (textField.completionActive) {
                        textField.commitCompletion()
                        event.accepted = true
                    }
                }

                Keys.onEscapePressed: {
                    event.accepted = true
                    if (textField.completionActive) {
                        listView.model = null
                    }
                }

                Keys.onUpPressed: {
                    listView.decrementCurrentIndex()
                    event.accepted = false
                }

                Keys.onDownPressed: {
                    listView.incrementCurrentIndex()
                    event.accepted = false
                }

                style: TextFieldStyle {
                    textColor: creatorTheme.PanelTextColorLight
                    padding.top: 6
                    padding.bottom: 2
                    padding.left: 6
                    placeholderTextColor: creatorTheme.PanelTextColorMid
                    background: Rectangle {
                        implicitWidth: 100
                        implicitHeight: 23
                        radius: 2
                        color: creatorTheme.QmlDesignerBackgroundColorDarker
                        border.color: creatorTheme.QmlDesignerBorderColor
                    }
                }
            }

            Row {
                spacing: 2
                Button {
                    width: 16
                    height: 16
                    style: ButtonStyle {
                        background: Item{
                            Image {
                                width: 16
                                height: 16
                                source: "image://icons/error"
                                opacity: {
                                    if (control.pressed)
                                        return 0.8;
                                    return 1.0;
                                }
                                Rectangle {
                                    z: -1
                                    anchors.fill: parent
                                    color: control.pressed || control.hovered ? creatorTheme.QmlDesignerBackgroundColorDarker : creatorTheme.QmlDesignerButtonColor
                                    border.color: creatorTheme.QmlDesignerBorderColor
                                    radius: 2
                                }
                            }
                        }
                    }
                    onClicked: {
                        backendValue.expression = textField.text
                        expressionDialog.visible = false
                    }
                }
                Button {
                    width: 16
                    height: 16
                    style: ButtonStyle {
                        background: Item {
                            Image {
                                width: 16
                                height: 16
                                source: "image://icons/ok"
                                opacity: {
                                    if (control.pressed)
                                        return 0.8;
                                    return 1.0;
                                }
                                Rectangle {
                                    z: -1
                                    anchors.fill: parent
                                    color: control.pressed || control.hovered ? creatorTheme.QmlDesignerBackgroundColorDarker : creatorTheme.QmlDesignerButtonColor
                                    border.color: creatorTheme.QmlDesignerBorderColor
                                    radius: 2
                                }
                            }
                        }
                    }
                    onClicked: {
                        expressionDialog.visible = false
                    }
                }
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.margins: 4
            }
        }
    }

}
