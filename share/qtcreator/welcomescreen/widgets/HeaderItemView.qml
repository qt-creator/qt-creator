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

import QtQuick 1.0

Item {
    id: root
    height: childrenRect.height
    property string header
    property QtObject model
    property Component delegate

    Rectangle {
        color: "#ececec"
        anchors.top: parent.top
        anchors.bottom: dataSection.top
        width: parent.width
    }
    Text {
        id: titleText
        text: root.header
        width: parent.width
        font.bold: true
        font.pointSize: 16
        color: "#444"
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        elide: Text.ElideRight
        anchors.topMargin: 10
        anchors.leftMargin: 10
    }
    Rectangle {
        height: 1
        color: "#ccc"
        anchors.bottom: dataSection.top
        width: parent.width
    }
    Rectangle {
        height: 1
        color: "#ccc"
        anchors.top: parent.top
        width: parent.width
    }

    Column {
        id: dataSection
        anchors.topMargin: 10
        anchors.top: titleText.bottom
        anchors.left: parent.left
        anchors.right: parent.right

        Repeater {
            model: root.model
            delegate: root.delegate
        }
    }
}
