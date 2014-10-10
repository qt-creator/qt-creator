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

Item {
    id: projectItem
    width: row.width + 8
    height: text.height

    Rectangle {
        anchors.fill: parent
        color: "#f9f9f9"
        visible: mouseArea.containsMouse
    }

    property alias projectName: projectNameText.text
    property alias projectPath: pathText.text

    Row {
        id: row
        spacing: 5

        Image {
            y: 3
            source: "images/project.png"
        }

        Column {
            id: text

            LinkedText {
                id: projectNameText
                height: 20
                font.underline: mouseArea.containsMouse
                font.pixelSize: fonts.linkFont.pixelSize
                enlargeMouseArea: false
            }
            NativeText {
                id: pathText
                height: 20
                color: "#6b6b6b"
                font: fonts.smallPath
            }
        }
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: projectWelcomePage.requestProject(filePath);
    }
}
