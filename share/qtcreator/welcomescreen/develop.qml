/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

import QtQuick 1.1
import "widgets"

Rectangle {
    id: rectangle1
    width: 900
    height: 600

    PageCaption {
        id: pageCaption

        x: 32
        y: 8

        anchors.rightMargin: 16
        anchors.right: parent.right
        anchors.leftMargin: 16
        anchors.left: parent.left

        caption: qsTr("Develop")
    }

    Item {
        id: canvas

        x: 12
        y: 0
        width: 1024

        anchors.bottomMargin: 0
        anchors.bottom: parent.bottom
        anchors.top: parent.top
        anchors.topMargin: 0

        RecentSessions {
            id: recentSessions

            x: 87
            y: 144
            width: 274

            anchors.right: recentlyUsedSessions.right
            anchors.rightMargin: -89
            anchors.top: recentlyUsedSessions.bottom
            anchors.topMargin: 20

            model: sessionList
        }

        Rectangle {
            width: 1
            height: line.height
            color: "#c4c4c4"
            anchors.left: recentSessions.right
            anchors.leftMargin: -1
            anchors.top: recentSessions.top

        }

        RecentProjects {
            x: 406
            y: 144
            width: 481
            height: 416
            id: recentProjects

            anchors.top: recentlyUsedProjects.bottom
            anchors.topMargin: 20
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 40
            anchors.right: parent.right
            anchors.rightMargin: 137

            model: projectList
        }

        Rectangle {
            id: line
            width: 1
            height: Math.min(recentProjects.contentHeight + 120, recentProjects.height)
            color: "#c4c4c4"
            anchors.left: recentProjects.right
            anchors.leftMargin: -1
            anchors.top: recentProjects.top
        }

        Text {
            id: recentlyUsedSessions

            x: pageCaption.x + pageCaption.textOffset
            y: 105

            color: "#535353"
            text: qsTr("Recently used sessions")
            font.pixelSize: 16
            font.family: "Helvetica"
            font.bold: true
        }

        Text {
            id: recentlyUsedProjects
            x: 406

            y: 105
            color: "#535353"
            text: qsTr("Recently used Projects")
            anchors.left: recentlyUsedSessions.right
            anchors.leftMargin: 134
            font.bold: true
            font.family: "Helvetica"
            font.pixelSize: 16
        }

        Item {
            id: actions

            x: 90
            y: 296
            width: 140
            height: 70

            anchors.topMargin: 42
            anchors.top: recentSessions.bottom

            LinkedText {
                id: openProject
                x: 51
                y: 45
                text: qsTr("Open Project")
                onClicked: welcomeMode.openProject();
            }

            LinkedText {
                id: createProject
                x: 51
                y: 13
                text: qsTr("Create Project")
                onClicked: welcomeMode.newProject();
            }

            Image {
                id: icon02
                x: 2
                y: 32
                source: "widgets/images/icons/openIcon.png"
            }

            Image {
                id: icon01
                x: 0
                y: 0
                source: "widgets/images/icons/createIcon.png"
            }
        }
    }
}
