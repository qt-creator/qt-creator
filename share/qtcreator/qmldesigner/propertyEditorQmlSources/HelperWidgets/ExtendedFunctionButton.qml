/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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
import QtQuick.Controls 1.0 as Controls
import QtQuick.Window 2.0

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
         color: "gray"
         radius: 4
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
         }

         Row {
             spacing: 6

         Controls.Button {
             text: "Apply"
             iconSource: "images/apply.png"
             onClicked: {
                 backendValue.expression = textField.text
                 expressionDialog.visible = false
             }
         }
         Controls.Button {
             text: "Cancel"
             iconSource: "images/cancel.png"
             onClicked: {
                 expressionDialog.visible = false
             }
         }
         anchors.right: parent.right
         anchors.bottom: parent.bottom
         }
     }

}
