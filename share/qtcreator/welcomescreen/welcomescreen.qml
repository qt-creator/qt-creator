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
import components 1.0 as Components

Rectangle {
    id: root
    color: "white"
    // work around the fact that we can't use
    // a property alias to welcomeMode.activePlugin
    property int current: 0
    onCurrentChanged: welcomeMode.activePlugin = current
    Component.onCompleted: current = welcomeMode.activePlugin

    BorderImage {
        id: inner_background
        Image {
            id: header;
            source: "qrc:welcome/images/center_frame_header.png";
            anchors.verticalCenter: parent.verticalCenter;
            anchors.horizontalCenter: parent.horizontalCenter;
            anchors.topMargin: 2
        }
        anchors.top: root.top
        source: "qrc:welcome/images/background_center_frame_v2.png"
        width: parent.width
        height: 60
        border.right: 2
        border.left: 2
        border.top: 2
        border.bottom: 10
    }

    LinksBar {
        id: navigationAndDevLinks
        property alias current: root.current
        anchors.top: inner_background.bottom
        anchors.left: news.right
        anchors.right: parent.right
        anchors.bottomMargin: 4
        anchors.topMargin: -2
        model: tabs.model
    }

    Rectangle {
        color: "#eee"
        id: news
        opacity: 0.7
        anchors.top: navigationAndDevLinks.top
        anchors.bottom: feedback.top
        anchors.left: parent.left
        width: 270
        FeaturedAndNewsListing {
            anchors.fill: parent
        }
        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            height: 1
            color: "black"
        }
        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.topMargin: 1
            height: 1
            color: "#ccc"
        }
        Rectangle{
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width:1
            color: "black"
        }
    }

    TabWidget {
        id: tabs
        property int current: root.current
        model: pagesModel
        anchors.top: navigationAndDevLinks.bottom
        anchors.bottom: feedback.top
        anchors.left: news.right
        anchors.right: parent.right
        anchors.leftMargin: 0
        anchors.rightMargin: 0
        anchors.margins: 4
    }

    Feedback {
        id: feedback
        anchors.bottom: parent.bottom
        width: parent.width
    }

}
