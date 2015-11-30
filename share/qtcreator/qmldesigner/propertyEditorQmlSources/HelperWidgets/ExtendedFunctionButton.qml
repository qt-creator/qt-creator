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
import QtQuick.Controls 1.0 as Controls
import QtQuick.Window 2.0
import QtQuick.Controls.Styles 1.1
import "Constants.js" as Constants

Item {
    width: 14
    height: 14

    id: extendedFunctionButton

    property variant backendValue

    property string icon:  "images/placeholder.png"
    property string hoverIcon:  "images/submenu.png";

    property bool active: true


    function setIcon() {
        if (backendValue == null) {
            extendedFunctionButton.icon = "images/placeholder.png"
        } else if (backendValue.isBound ) {
            if (backendValue.isTranslated) { //translations are a special case
                extendedFunctionButton.icon = "images/placeholder.png"
            } else {
                extendedFunctionButton.icon = "images/expression.png"
            }
        } else {
            if (backendValue.complexNode != null && backendValue.complexNode.exists) {
                //extendedFunctionButton.icon = "images/behaivour.png"
            } else {
                extendedFunctionButton.icon = "images/placeholder.png"
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
        x: 10
        color: "#424242"

        radius: 3
        border.color: "black"
        gradient: Gradient {
            GradientStop {color: "#2c2c2c" ; position: 0}
            GradientStop {color: "#343434" ; position: 0.15}
            GradientStop {color: "#373737" ; position: 1.0}
        }

        id: expressionDialog

        onVisibleChanged: {
            var pos  = itemPane.mapFromItem(extendedFunctionButton.parent, 0, 0);
            y = pos.y + 2;
        }

        width: parent.width - 20
        height: 120

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
                textColor: Constants.colorsDefaultText
                padding.top: 3
                padding.bottom: 1
                padding.left: 16
                placeholderTextColor: "gray"
                background: Rectangle {
                    implicitWidth: 100
                    implicitHeight: 23
                    radius: 3
                    gradient: Gradient {
                        GradientStop {color: "#2c2c2c" ; position: 0}
                        GradientStop {color: "#343434" ; position: 0.15}
                        GradientStop {color: "#373737" ; position: 1.0}
                    }
                }
            }
        }

        Row {
            spacing: 0
            Button {
                style: ButtonStyle {
                    background: Image {
                        source: "images/apply.png"
                        Rectangle {
                            opacity:  control.pressed ? 0.5 : 0
                            anchors.fill: parent
                            gradient: Gradient {
                                GradientStop {color: "#606060" ; position: 0}
                                GradientStop {color: "#404040" ; position: 0.07}
                                GradientStop {color: "#303030" ; position: 1}
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
                style: ButtonStyle {
                    background: Image {
                        source: "images/cancel.png"

                        Rectangle {
                            opacity:  control.pressed ? 0.5 : 0
                            anchors.fill: parent
                            gradient: Gradient {
                                GradientStop {color: "#606060" ; position: 0}
                                GradientStop {color: "#404040" ; position: 0.07}
                                GradientStop {color: "#303030" ; position: 1}
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
        }
    }

}
