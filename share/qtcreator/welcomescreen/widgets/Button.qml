/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

import Qt 4.7

BorderImage {
    property string text
    property string image
    property int iconSize : innerImg.sourceSize.height
    signal clicked;
    id: root
    source: "qrc:/welcome/images/btn_26.png"
    height: innerImg.height + 16

    border.left: 5; border.top: 5
    border.right: 5; border.bottom: 5

    Image{
        id: innerImg
        height: root.iconSize
        width: root.iconSize
        anchors.verticalCenter: label.verticalCenter
        anchors.right: label.left
        anchors.rightMargin: 4
        visible: root.image != ""
        source: root.image
    }

    Text {
        id: label;
        anchors.left: innerImg.right
        anchors.right: parent.right
        text: root.text
    }

    Keys.onSpacePressed:clicked()
    MouseArea { id: mouseArea; anchors.fill: parent; hoverEnabled: true; onClicked: root.clicked() }

    states: [
        State {
            id: pressedState; when: mouseArea.pressed;
            PropertyChanges { target: root; source: "qrc:/welcome/images/btn_26_pressed.png" }
        },
        State {
            id: hoverState; when: mouseArea.containsMouse
            PropertyChanges { target: root; source: "qrc:/welcome/images/btn_26_hover.png" }
        }
    ]
}
