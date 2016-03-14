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
    property string iconSource
    property string title: "title"
    property string openUrl
    property string openHelpUrl
    height: 30
    width: 231

    color: mouseArea.containsMouse
           ? creatorTheme.Welcome_HoverColor
           : creatorTheme.Welcome_BackgroundColor

    Image {
        id: image
        width: 16
        height: 16
        x: 34
        source: iconSource
        anchors.verticalCenter: parent.verticalCenter
    }
    NativeText {
        text: title
        anchors.verticalCenter: parent.verticalCenter
        anchors.left: image.right
        anchors.leftMargin: 8
        color: creatorTheme.Welcome_TextColor
        font.pixelSize: 11
        font.underline: mouseArea.containsMouse
    }
    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        onClicked: {
            if (openUrl)
                gettingStarted.openUrl(openUrl);
            if (openHelpUrl)
                gettingStarted.openHelp(openHelpUrl);
        }
    }
}

