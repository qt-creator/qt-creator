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
import components 1.0 as Components

Item {
    id: root
    property int margin: 10

    Components.ScrollArea {
        id: scrollArea
        anchors.fill: parent
        frame: false
        Item {
            height: Math.max(recentSessions.height + manageSessionsButton.height + margin,
                             recentProjects.height)
            width: root.width
            Widgets.RecentSessions {
                id: recentSessions
                width: parent.width / 3 - margin
            }
            Widgets.Button {
                id: manageSessionsButton
                anchors.top: recentSessions.bottom
                anchors.topMargin: margin
                anchors.left: recentSessions.left
                text: qsTr("Manage Sessions...")
                onClicked: projectWelcomePage.manageSessions()
            }

            Widgets.RecentProjects {
                id: recentProjects
                x: parent.width / 3 + margin
                width: parent.width - x
            }
        }
    }
}


