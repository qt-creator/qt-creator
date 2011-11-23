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
import QtQuick 1.0

Rectangle {
    id: gettingStartedRoot
    width: 920
    height: 600

    PageCaption {
        id: pageCaption

        x: 32
        y: 8

        anchors.rightMargin: 16
        anchors.right: parent.right
        anchors.leftMargin: 16
        anchors.left: parent.left

        caption: qsTr("Getting Started")
    }

    Item {
        id: canvas

        width: 920
        height: 200

        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: 0

        Feedback {
            id: feedback

            x: 851
            y: 424
            anchors.right: parent.right
            anchors.rightMargin: 22

        }

        GettingStartedItem {
            x: 688
            y: 83

            anchors.rightMargin: 32
            anchors.right: parent.right

            description: qsTr("To select a tutorial and learn how to develop applications.")
            title: qsTr("Start Developing")
            number: 4
            imageUrl: "widgets/images/gettingStarted04.png"
            onClicked: tab.currentIndex = 3;

        }

        GettingStartedItem {
            x: 468
            y: 83

            description: qsTr("To check that the Qt SDK installation was successful, open an example application and run it.")
            title: qsTr("Building and Running an Example Application")
            number: 3
            imageUrl: "widgets/images/gettingStarted03.png"
            onClicked: gettingStarted.openSplitHelp("qthelp://com.nokia.qtcreator/doc/creator-build-example-application.html")
        }

        GettingStartedItem {
            id: gettingStartedItem

            x: 30
            y: 83

            imageUrl: "widgets/images/gettingStarted01.png"
            title: qsTr("IDE Overview")
            description: qsTr("To find out what kind of integrated enviroment (IDE) Qt Creator is.")
            number: 1

            onClicked: gettingStarted.openHelp("qthelp://com.nokia.qtcreator/doc/creator-overview.html")
        }

        GettingStartedItem {
            x: 250
            y: 83

            description: qsTr("To become familar with the parts of the Qt Creator user interface and to learn how to use them.")
            title: qsTr("User Interface")
            imageUrl: "widgets/images/gettingStarted02.png"
            number: 2
            onClicked: gettingStarted.openHelp("qthelp://com.nokia.qtcreator/doc/creator-quick-tour.html")
        }

        Grid {
            id: grid

            x: 36
            y: 424

            spacing: 24

            rows: gettingStartedRoot.height > 640 ? 3 : 1

            Row {
                id: userGuide
                spacing: 4
                Image {
                    y: 10
                    source: "widgets/images/icons/userguideIcon.png"
                }

                LinkedText {
                    x: 19
                    y: 5
                    height: 38
                    text: qsTr("User Guide")
                    verticalAlignment: Text.AlignBottom
                    onClicked: gettingStarted.openHelp("qthelp://com.nokia.qtcreator/doc/index.html")
                }
            }

            Row {
                id: onlineCommunity
                spacing: 4
                Image {
                    y: 10
                    source: "widgets/images/icons/communityIcon.png"
                }

                LinkedText {
                    x: 19
                    y: 5
                    height: 38
                    text: qsTr("Online Community")
                    verticalAlignment: Text.AlignBottom
                    onClicked: gettingStarted.openUrl("http://developer.qt.nokia.com/forums")
                }
            }

            Row {
                id: labs
                spacing: 4
                Image {
                    y: 10
                    source: "widgets/images/icons/labsIcon.png"
                }

                LinkedText {
                    x: 19
                    y: 5
                    height: 38
                    text: qsTr("Labs")
                    verticalAlignment: Text.AlignBottom
                    onClicked: gettingStarted.openUrl("http://labs.qt.nokia.com")
                }

            }
        }

        Image {
            x: 231
            y: 155
            source: "widgets/images/arrowBig.png"
        }

        Image {
            x: 451
            y: 155
            source: "widgets/images/arrowBig.png"
        }

        Image {
            x: 669
            y: 155
            source: "widgets/images/arrowBig.png"
        }

    }
}
