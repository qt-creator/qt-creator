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
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

import QtQuick 2.1

Item {
    id: projectItem
    width: row.width + 8
    height: text.height

    Rectangle { // background shown on hover over project item
        anchors.fill: parent
        color: creatorTheme.Welcome_ProjectItem_BackgroundColorHover
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
                font.family: fonts.linkFont.family
                enlargeMouseArea: false
            }
            NativeText {
                id: pathText
                height: 20
                color: creatorTheme.Welcome_ProjectItem_TextColorFilepath
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
