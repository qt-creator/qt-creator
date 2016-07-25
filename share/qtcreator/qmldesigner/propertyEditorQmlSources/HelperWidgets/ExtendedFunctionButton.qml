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
            }
        }
    }

    Rectangle {
        parent: itemPane
        visible: false
        x: 6

        id: expressionDialog

        onVisibleChanged: {
            var pos  = itemPane.mapFromItem(extendedFunctionButton.parent, 0, 0);
            y = pos.y + 2;
        }

        width: parent.width - 12
        height: 120

        radius: 2
        color: creatorTheme.QmlDesignerBackgroundColorDarkAlternate
        //border.color: creatorTheme.QmlDesignerBackgroundColorDarker
        border.color: creatorTheme.QmlDesignerTabLight

        Controls.TextField {
            id: textField
            anchors.fill: parent
            anchors.leftMargin: 4
            anchors.rightMargin: 4
            anchors.topMargin: 4
            anchors.bottomMargin: 20
            onAccepted: {
                backendValue.expression = textField.text
                expressionDialog.visible = false
            }

            style: TextFieldStyle {
                textColor: creatorTheme.PanelTextColorLight
                padding.top: 3
                padding.bottom: 1
                padding.left: 16
                placeholderTextColor: creatorTheme.PanelTextColorMid
                background: Rectangle {
                    implicitWidth: 100
                    implicitHeight: 23
                    radius: 2
                    color: creatorTheme.QmlDesignerBackgroundColorDarkAlternate
                }
            }
        }

        Row {
            spacing: 0
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
                                    return 0.5;

                                if (control.hovered)
                                    return 1.0;

                                return 0.9;
                            }
                            Rectangle {
                                z: -1
                                anchors.fill: parent
                                color: control.hovered? creatorTheme.FancyToolButtonSelectedColor : creatorTheme.BackgroundColorDark
                                border.color: creatorTheme.QmlDesignerBackgroundColorDarker
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
                                    return 0.5;

                                if (control.hovered)
                                    return 1.0;

                                return 0.9;
                            }
                            Rectangle {
                                z: -1
                                anchors.fill: parent
                                color: control.hovered? creatorTheme.FancyToolButtonSelectedColor : creatorTheme.BackgroundColorDark
                                border.color: creatorTheme.QmlDesignerBackgroundColorDarker
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
            anchors.margins: 2
        }
    }

}
