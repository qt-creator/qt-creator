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
import widgets 1.0

Item  {
    id: root

    property var fonts: CustomFonts {}

    property int screenDependHeightDistance: Math.min(50, Math.max(16, height / 30))

    DropArea {
        anchors.fill: parent
        keys: ["text/uri-list"]
        onDropped: {
            if ((drop.supportedActions & Qt.CopyAction != 0)
                    && welcomeMode.openDroppedFiles(drop.urls))
                drop.accept(Qt.CopyAction);
        }
    }

    SideBar {
        id: sideBar
        model: pagesModel
        anchors.top: parent.top
        anchors.bottom: parent.bottom
    }

    Rectangle {
        id: splitter
        color: creatorTheme.Welcome_DividerColor // divider between left and right pane
        width: 1
        anchors.top: parent.top
        anchors.bottom: parent.bottom

        anchors.left: sideBar.right

    }

    PageLoader {
        id: loader

        model: pagesModel
        currentIndex: sideBar.currentIndex

        anchors.top: parent.top
        anchors.left: splitter.right
        anchors.right: parent.right
        anchors.bottom: parent.bottom

        color: creatorTheme.Welcome_BackgroundColor
    }
}
