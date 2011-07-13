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
import components 1.0 as Components

HeaderItemView {
    clip: true
    header: qsTr("Recently Used Sessions")
    model: sessionList

    delegate: Rectangle {
        height: 60
        width: dataSection.width

        Rectangle {
            height: 1
            color: "#eee"
            anchors.top: parent.top
            width: parent.width
        }
        color: mousearea.containsMouse ? "#f9f9f9" : "white"

        function fullSessionName()
        {
            var newSessionName = sessionName
            if (model.currentSession)
                newSessionName = qsTr("%1 (current session)").arg(newSessionName);
            return newSessionName;
        }

        Image{
            id: arrowImage;
            source: "qrc:welcome/images/list_bullet_arrow.png";
            anchors.verticalCenter: parent.verticalCenter;
            anchors.left: parent.left
        }

        Text {
            Components.QStyleItem { id: styleItem; cursor: "pointinghandcursor"; anchors.fill: parent }
            id: fileNameText
            text: parent.fullSessionName()
            font.italic: model.defaultSession
            elide: Text.ElideMiddle
            anchors.left: arrowImage.right
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            anchors.leftMargin: 10
            elide: Text.ElideRight
        }

        Timer { id: timer; interval: 500; onTriggered: styleItem.showToolTip(sessionName) }

        MouseArea {
            id: mouseare
            anchors.fill: parent
            onClicked: projectWelcomePage.requestSession(sessionName)
            hoverEnabled: true
            onEntered:timer.start()
            onExited: timer.stop()
        }
    }
}
