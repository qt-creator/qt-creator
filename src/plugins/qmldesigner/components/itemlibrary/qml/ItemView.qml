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

// view displaying an item library grid item

Item {
    id: itemView

    // public

    signal itemPressed()
    signal itemDragged()

    // internal

    ItemsViewStyle { id: style }

    // frame
    Rectangle {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 1
        color: style.gridLineLighter
    }
    Rectangle {
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: 1
        color: style.gridLineDarker
    }
    Rectangle {
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        width: 1
        color: style.gridLineLighter
    }
    Rectangle {
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        width: 1
        color: style.gridLineDarker
    }
    Rectangle {
        anchors.top:parent.top
        anchors.right:parent.right
        width:1
        height:1
        color: style.backgroundColor
    }
    Rectangle {
        anchors.bottom:parent.bottom
        anchors.left:parent.left
        width:1
        height:1
        color: style.backgroundColor
    }

    Image {
        id: itemIcon

        anchors.top: parent.top
        anchors.topMargin: style.cellVerticalMargin
        anchors.horizontalCenter: parent.horizontalCenter

        width: itemLibraryIconWidth  // to be set in Qml context
        height: itemLibraryIconHeight   // to be set in Qml context
        source: itemLibraryIconPath     // to be set by model
    }

    Text {
        id: text
        elide: Text.ElideMiddle
        anchors.top: itemIcon.bottom
        anchors.topMargin: style.cellVerticalSpacing
        anchors.left: parent.left
        anchors.leftMargin: style.cellHorizontalMargin
        anchors.right: parent.right
        anchors.rightMargin: style.cellHorizontalMargin
        width: style.textWidth
        height: style.textHeight

        verticalAlignment: "AlignVCenter"
        horizontalAlignment: "AlignHCenter"
        text: itemName  // to be set by model
        color: style.itemNameTextColor
    }

    MouseArea {
        id: mouseRegion
        anchors.fill: parent

        property bool reallyPressed: false
        property int pressedX
        property int pressedY

        onPressed: {
            reallyPressed = true
            pressedX = mouse.x
            pressedY = mouse.y
            itemPressed()
        }
        onPositionChanged: {
            if (reallyPressed &&
                (Math.abs(mouse.x - pressedX) +
                 Math.abs(mouse.y - pressedY)) > 3) {
                itemDragged()
                reallyPressed = false
            }
        }
        onReleased: reallyPressed = false
    }
}

