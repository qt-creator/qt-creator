/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

import QtQuick 1.1
import widgets 1.0

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

        Rectangle {
            width: 1
            height: Math.max(Math.min(recentProjects.contentHeight + 120, recentProjects.height), sessions.height)
            color: "#c4c4c4"
            anchors.left: sessions.right
            anchors.leftMargin: -1
            anchors.top: sessions.top
            visible: !sessions.scrollBarVisible
            id: sessionLine
        }

        RecentProjects {
            x: 406
            y: 144
            width: 481
            height: 432
            id: recentProjects

            anchors.left: recentProjectsTitle.left

            anchors.top: recentProjectsTitle.bottom
            anchors.topMargin: 20
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 40
            anchors.right: parent.right
            anchors.rightMargin: 80

            model: projectList
        }

        Rectangle {
            id: line
            width: 1
            height: sessionLine.height
            color: "#c4c4c4"
            anchors.left: recentProjects.right
            anchors.leftMargin: -1
            anchors.top: recentProjects.top
            visible: !recentProjects.scrollBarVisible
        }

        Text {
            id: sessionsTitle

            x: pageCaption.x + pageCaption.textOffset
            y: 105

            color: "#535353"
            text: qsTr("Sessions")
            font.pixelSize: 16
            font.family: "Helvetica"
            font.bold: true
        }

        Text {
            id: recentProjectsTitle
            x: 406

            y: 105
            color: "#535353"
            text: qsTr("Recent Projects")
            anchors.left: sessionsTitle.right
            anchors.leftMargin: 280
            font.bold: true
            font.family: "Helvetica"
            font.pixelSize: 16
        }

        Item {
            id: actions
            x: pageCaption.x + pageCaption.textOffset

            y: 295
            width: 140
            height: 70

            anchors.topMargin: 42
            anchors.top: sessions.bottom

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
                y: 32
                source: "widgets/images/icons/openIcon.png"
            }

            Image {
                id: icon01
                source: "widgets/images/icons/createIcon.png"
            }
        }

        Sessions {
            id: sessions

            x: 87
            y: 144
            width: 274

            anchors.left: sessionsTitle.left
            anchors.right: recentProjectsTitle.left
            anchors.rightMargin: 40
            anchors.top: sessionsTitle.bottom
            anchors.topMargin: 20

            model: sessionList
        }
    }
}
