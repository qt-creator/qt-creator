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
import qtcomponents 1.0 as Components

Rectangle {
    id: root
    width: 1024
    height: 768
    color: "white"
    // work around the fact that we can't use
    // a property alias to welcomeMode.activePlugin
    property int current: 0
    onCurrentChanged: welcomeMode.activePlugin = current
    Component.onCompleted: current = welcomeMode.activePlugin


    LinksBar {
        id: navigationAndDevLinks
        property alias current: root.current
        anchors.topMargin: -1
        anchors.top: inner_background.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottomMargin: 4
        model: tabs.model
    }


    TabWidget {
        id: tabs
        property int current: root.current
        anchors.rightMargin: 0
        anchors.leftMargin: 0
        model: pagesModel
        anchors.top: feedback.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: news.left
        anchors.margins: 0
    }

    Item {
        Image {
            source: "qrc:welcome/images/welcomebg.png"
            anchors.fill: parent
            opacity: 0.5
        }

        anchors.right: parent.right
        id: news
        opacity: 0.7
        anchors.top: navigationAndDevLinks.bottom
        anchors.bottom: parent.bottom
        width: 220
        anchors.topMargin: -1

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
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: 1
            color: "black"
            anchors.left: parent.left
        }
    }

    Feedback {
        id: feedback
        height: 38
        anchors.top: navigationAndDevLinks.bottom
        anchors.left: parent.left
        anchors.right: news.left
        searchVisible: tabs.currentHasSearchBar
    }

    BorderImage {
        id: inner_background
        x: 0
        y: 0
        anchors.top: root.top
        source: "qrc:welcome/images/background_center_frame_v2.png"
        width: parent.width
        height: 0
        anchors.topMargin: 0
        border.right: 2
        border.left: 2
        border.top: 2
        border.bottom: 10
    }
}
