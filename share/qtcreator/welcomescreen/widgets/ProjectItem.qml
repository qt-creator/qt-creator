/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

import QtQuick 2.1

Rectangle {
    id: projectItem
    width: Math.max(projectNameText.width, pathText.width) + projectNameText.x + 11
    height: 48

    color: mouseArea.containsMouse
           ? creatorTheme.Welcome_HoverColor
           : creatorTheme.Welcome_BackgroundColor

    property alias projectName: projectNameText.text
    property alias projectPath: pathText.text

    Image {
        id: icon
        x: 11
        y: 6
        height: 16
        width: 16
        source: "image://icons/project/Welcome_ForegroundSecondaryColor"
    }

    NativeText {
        x: 38
        id: projectNameText
        font.pixelSize: fonts.linkFont.pixelSize
        font.family: fonts.linkFont.family
        font.underline: mouseArea.containsMouse
        color: creatorTheme.Welcome_LinkColor
        anchors.verticalCenter: icon.verticalCenter
    }

    NativeText {
        id: pathText
        anchors.left: projectNameText.left
        anchors.bottom: projectItem.bottom
        anchors.bottomMargin: 6
        color: creatorTheme.Welcome_ForegroundPrimaryColor
        font: fonts.smallPath
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        onClicked: projectWelcomePage.requestProject(filePath);
    }
}
