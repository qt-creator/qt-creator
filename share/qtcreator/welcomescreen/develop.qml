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
import "widgets" as Widgets
import qtcomponents 1.0 as Components

Item {
    id: root
    property int margin: 8

    Components.ScrollArea {
        id: scrollArea
        anchors.fill:  parent
        anchors.margins: - margin
        frame: false
        Item {
            id: baseitem
            height: Math.max(recentSessions.height, recentProjects.height)
            width: root.width
            Widgets.RecentSessions {
                id: recentSessions
                width: Math.floor(root.width / 2.5)
                anchors.left: parent.left
            }
            Widgets.RecentProjects {
                id: recentProjects
                anchors.left:  recentSessions.right
                anchors.right: parent.right
                anchors.rightMargin: scrollArea.verticalScrollBar.visible ? 0 :
                                         -scrollArea.verticalScrollBar.width
            }

        }
    }
    Rectangle {
        anchors.top: scrollArea.top
        height: root.height + 2 * margin
        width: 1
        color: "#ccc"
        x: recentProjects.x - margin
    }
}
