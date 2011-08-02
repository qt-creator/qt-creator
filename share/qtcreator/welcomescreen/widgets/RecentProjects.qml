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

HeaderItemView {
    header: qsTr("Recently Edited Projects")
    model: projectList
    delegate: Rectangle {
        Components.QStyleItem { id: styleItem; cursor: "pointinghandcursor"; anchors.fill: parent }
        height: 60
        width: dataSection.width

        Rectangle {
            height: 1
            color: "#eee"
            anchors.bottom: parent.bottom
            width: parent.width
        }
        color: mousearea.containsMouse ? "#f9f9f9" : "white"

        Image {
            id: arrowImage;
            source: "qrc:welcome/images/list_bullet_arrow.png";
            anchors.verticalCenter: parent.verticalCenter;
            anchors.left: parent.left
            anchors.leftMargin: 10
        }

        Column {
            spacing: 4
            anchors.left: arrowImage.right
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            Text {
                id: nameText
                text: displayName
                font.bold: true
                width: parent.width
                anchors.left: parent.left
                anchors.leftMargin: 10
                anchors.rightMargin: arrowImage.width + 10
                anchors.right: parent.right
                elide: Text.ElideRight
            }

            Text {
                id: filepath
                text: prettyFilePath
                elide: Text.ElideMiddle
                color: "grey"
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.leftMargin: 10
                anchors.rightMargin: arrowImage.width + 14
            }
        }

        Timer {
            id: timer
            interval: 1000
            onTriggered: {
                if (filepath.truncated)
                    styleItem.showToolTip(filePath)
            }
        }

        MouseArea {
            id: mousearea
            anchors.fill: parent
            onClicked: projectWelcomePage.requestProject(filePath)
            hoverEnabled: true
            onEntered:timer.start()
            onExited: timer.stop()
        }
    }
}
