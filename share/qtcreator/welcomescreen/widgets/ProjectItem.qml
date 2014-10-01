/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

import QtQuick 2.1

Item {
    id: projectItem
    width: projectList.width
    height: 32

    Rectangle {
        anchors.fill: parent
        color: "#f9f9f9"
        opacity: projectNameText.hovered ? 1 : 0
    }

    property alias projectName: projectNameText.text
    property alias projectPath: pathText.text

    Image {
        source: "images/project.png"
        anchors.verticalCenter: projectNameText.verticalCenter
        width: 12
        height: 12
    }

    LinkedText {
        id: projectNameText
        y: 2
        anchors.left: parent.left
        anchors.leftMargin: 7 + 12
        anchors.right: parent.right
        anchors.rightMargin: 4
        onClicked: projectWelcomePage.requestProject(filePath)
    }

    NativeText {
        id: pathText
        y: 18
        color: "#6b6b6b"
        anchors.right: parent.right
        anchors.rightMargin: 12
        anchors.left: parent.left
        anchors.leftMargin: 7 + 12
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
