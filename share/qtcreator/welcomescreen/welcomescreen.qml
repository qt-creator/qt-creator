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
import "widgets"

Rectangle {
    id: root
    color: "#F2F2F2"
    // work around the fact that we can't use
    // a property alias to welcomeMode.activePlugin
    property int current: 0
    onCurrentChanged: welcomeMode.activePlugin = current
    Component.onCompleted: current = welcomeMode.activePlugin
    Item {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: feedback.top
        anchors.margins: 10

        Item {
            id: news
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.rightMargin: 5
            width: 270
            Rectangle {
                anchors.fill: parent
                border.color: "#36295B7F"
                border.width: 1
                color: "#B3FFFFFF"
            }
            FeaturedAndNewsListing {
                anchors.fill: parent
                anchors.margins: 8
            }
        }

        Image {
            id: tabFrame
            source: "qrc:welcome/images/welcomebg.png"
            smooth: true
            Rectangle {
                anchors.fill: parent
                border.color: "#36295B7F"
                border.width: 1
                color: "#20FFFFFF"
            }

            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.left: news.right
            anchors.right: parent.right
            anchors.leftMargin: 5

            LinksBar {
                id: linksBar
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                property alias current: root.current
                model: tabs.model
            }

            Rectangle {
                anchors.fill: linksBar
                anchors.bottomMargin: 1
                opacity: 1
                border.color: "#4D295B7F"
                border.width: 1
                color: "#00000000"
            }

            TabWidget {
                id: tabs
                property int current: root.current
                model: pagesModel
                anchors.top: linksBar.bottom
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.margins: 10
            }
        }
    }

    Feedback {
        id: feedback
        anchors.bottom: parent.bottom
        width: parent.width
    }

}
