/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
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
**
**************************************************************************/

import QtQuick 1.0

Item {
    id: projectItem
    width: 480
    height: 32

    Rectangle {
        anchors.fill: parent
        color: "#f9f9f9"
        opacity: projectNameText.hovered ? 1 : 0
    }

    property alias projectName: projectNameText.text
    property alias projectPath: pathText.text

    Image {
        source: "images/bullet.png"
        anchors.verticalCenter: projectNameText.verticalCenter
    }

    CustomFonts {
        id: fonts
    }

    LinkedText {
        id: projectNameText
        y: 2
        anchors.left: parent.left
        anchors.leftMargin: 8
        anchors.right: parent.right
        anchors.rightMargin: 4
        onClicked: projectWelcomePage.requestProject(filePath)
    }

    Text {
        id: pathText
        y: 18
        color: "#6b6b6b"
        anchors.right: parent.right
        anchors.rightMargin: 12
        anchors.left: parent.left
        anchors.leftMargin: 8
        font: fonts.smallPath
        elide: Text.ElideRight
        MouseArea {
            anchors.fill: parent
            hoverEnabled: true
            onEntered: {
                toolTip.show();
            }
            onExited: {
                toolTip.hide()
            }

        }
        ToolTip {
            x: 10
            y: 20
            id: toolTip
            text: pathText.text
        }
    }
}
