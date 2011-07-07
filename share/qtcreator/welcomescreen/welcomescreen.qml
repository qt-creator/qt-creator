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

Image {
    id: root
    source: "qrc:welcome/images/welcomebg.png"
    smooth: true

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

        BorderImage {
            id: news
            opacity: 0.7
            source: "qrc:welcome/images/rc_combined.png"
            border.left: 5; border.top: 5
            border.right: 5; border.bottom: 5
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.rightMargin: 5
            width: 270
            FeaturedAndNewsListing {
                anchors.fill: parent
                anchors.margins: 4
            }
        }

        BorderImage {
            id: tabFrame
            source: "qrc:welcome/images/rc_combined_transparent.png"
            border.left: 5; border.top: 5
            border.right: 5; border.bottom: 5

            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.left: news.right
            anchors.right: parent.right
            anchors.leftMargin: 5

            Rectangle {
                id: tabBar
                color: "#00000000"
                border.color: "black"
                border.width: 1
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.rightMargin: 1
                height: navigationAndDevLinks.height - 1

                LinksBar {
                    id: navigationAndDevLinks
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.leftMargin: 1
                    property alias current: root.current
                    model: tabs.model
                    tabBarWidth: width
                }
            }

            TabWidget {
                id: tabs
                property int current: root.current
                model: pagesModel
                anchors.top: tabBar.bottom
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
