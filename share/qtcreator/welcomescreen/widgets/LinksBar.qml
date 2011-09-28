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

import QtQuick 1.1
import qtcomponents 1.0 as Components

Item {
    id: tabBar

    height: 62
    width: parent.width
    property alias model: tabs.model
    property int tabWidth: Math.floor(tabBar.width/tabs.count)
    property url inactiveSource: "qrc:welcome/images/tab_inactive.png"
    property url activeSource: "qrc:welcome/images/tab_active.png"

    Row {
        id: row
        height: 24
        width: parent.width

        BorderImage {
            id: active
            width: 100
            height: parent.height
            border { top: 2; bottom: 2; left: 2; right: 2}
            source: activeSource
            Text {
                id: text
                horizontalAlignment: Qt.AlignHCenter; verticalAlignment: Qt.AlignVCenter
                anchors.fill: parent
                text: qsTr("Qt Creator")
                elide: Text.ElideRight
                color: "black"
            }
        }

        BorderImage {
            anchors.left: active.right
            anchors.right: parent.right
            height: parent.height
            border { top: 2; bottom: 2; left: 2; right: 2}
            source: inactiveSource
        }
    }
    Rectangle {
        id: background
        height: 38
        gradient: Gradient {
            GradientStop {
                position: 0
                color: "#e4e4e4"
            }

            GradientStop {
                position: 1
                color: "#cecece"
            }
        }
        width: parent.width
        anchors.top: row.bottom
        Rectangle {
            color: "black"
            height: 1
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
        }
    }

    Row {
        height: background.height
        anchors.top: row.bottom
        anchors.topMargin: 6
        width: parent.width
        Repeater {
            id: tabs
            height: parent.height
            model: tabBar.model
            delegate: SingleTab { }
        }
    }
}
