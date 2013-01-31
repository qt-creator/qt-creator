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

import QtQuick 1.0
import "custom" as Components

Components.SpinBox {
    id:spinbox

    property variant __upRect;
    property variant __downRect;
    property int __margin: (height -16)/2
    property string hint

    // Align height with button
    topMargin:__margin
    bottomMargin:__margin

    leftMargin:6
    rightMargin:6

    QStyleItem { id:edititem ; elementType:"edit" ; visible:false }
    property int buttonHeight:  edititem.sizeFromContents(70, 20).height
    property int buttonWidth: edititem.sizeFromContents(70, 20).width

    height: buttonHeight
    width: buttonWidth
    clip:false

    background: Item {
        anchors.fill: parent
        property variant __editRect

        Rectangle {
            id: editBackground
            x: __editRect.x - 1
            y: __editRect.y
            width: __editRect.width + 1
            height: __editRect.height
        }

        Item {
            id: focusFrame
            anchors.fill: editBackground
            visible: frameitem.styleHint("focuswidget")
            QStyleItem {
                id: frameitem
                anchors.margins: -6
                anchors.leftMargin: -6
                anchors.rightMargin: -7
                anchors.fill: parent
                visible: spinbox.activeFocus
                elementType: "focusframe"
            }
        }

        function updateRect() {
            __upRect = styleitem.subControlRect("up");
            __downRect = styleitem.subControlRect("down");
            __editRect = styleitem.subControlRect("edit");
            spinbox.leftMargin = __editRect.x + 2
            spinbox.rightMargin = spinbox.width -__editRect.width - __editRect.x
        }

        Component.onCompleted: updateRect()
        onWidthChanged: updateRect()
        onHeightChanged: updateRect()

        QStyleItem {
            id: styleitem
            anchors.fill: parent
            elementType: "spinbox"
            sunken: (downEnabled && downPressed) | (upEnabled && upPressed)
            hover: containsMouse
            focus: spinbox.focus
            enabled: spinbox.enabled
            value: (upPressed ? 1 : 0)           |
                   (downPressed == 1 ? 1<<1 : 0) |
                   (upEnabled ? (1<<2) : 0)      |
                   (downEnabled == 1 ? (1<<3) : 0)
            hint: spinbox.hint
        }
    }

    up: Item {
        x: __upRect.x
        y: __upRect.y
        width: __upRect.width
        height: __upRect.height
    }

    down: Item {
        x: __downRect.x
        y: __downRect.y
        width: __downRect.width
        height: __downRect.height
    }
}
