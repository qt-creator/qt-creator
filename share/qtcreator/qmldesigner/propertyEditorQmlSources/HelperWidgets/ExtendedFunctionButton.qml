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
            height: 160

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
                verticalAlignment: Text.AlignTop
                id: textField
                anchors.fill: parent
                anchors.leftMargin: 8
                anchors.rightMargin: 8
                anchors.topMargin: 24
                anchors.bottomMargin: 32
                onAccepted: {
                    backendValue.expression = textField.text
                    expressionDialog.visible = false
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
