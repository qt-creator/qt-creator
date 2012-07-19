/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
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
**
**************************************************************************/

import QtQuick 1.0
import widgets 1.0

Rectangle {
    width: 920
    height: 600
    color: "#edf0f2"
    id: root

    Rectangle {
        id: canvas


        opacity: 0

        Component.onCompleted: canvas.opacity = 1

        Behavior on opacity {
            PropertyAnimation {
                duration: 450
            }
        }

        width: Math.min(1024, parent.width)
        //this is a workaround for QTCREATORBUG-6803
        anchors.topMargin: (root.height > 700) ? 0 : 0

        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter

        PageLoader {
            anchors.fill: parent
            anchors.topMargin: 76
            model: pagesModel
        }

        CustomTab {
            id: tab
            x: 578
            y: 96
            anchors.right: parent.right
            anchors.rightMargin: 36
            model: pagesModel

        }

        Logo {
            x: 25
            y: 14
        }

        Rectangle {
            visible: root.width > 1042
            width: 2
            color: "#cdcdcd"
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.bottom: parent.bottom
        }

        Rectangle {
            visible: root.width > 1042
            width: 2
            color: "#cdcdcd"
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
        }

    }
}
