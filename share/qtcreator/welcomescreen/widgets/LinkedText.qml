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

import QtQuick 1.1
import qtcomponents 1.0

Text {
    id: root
    height: 16
    color: active ? "#f0f0f0" : colors.linkColor
    verticalAlignment: Text.AlignVCenter

    font: fonts.linkFont

    signal clicked
    signal entered
    signal exited

    property bool active: false

    property bool hovered: mouseArea.state === "hovered"
    onActiveChanged: {
        if (active)
            mouseArea.state = ""
    }

    property bool enlargeMouseArea: true

    CustomFonts {
        id: fonts
    }

    CustomColors {
        id: colors
    }

    Rectangle {
        color: "#909090"
        radius: 6
        opacity: root.active
        z: -1
        anchors.rightMargin: -6
        anchors.leftMargin: -6
        anchors.bottomMargin: -4
        anchors.topMargin: -4
        anchors.fill: parent
    }

    Rectangle {
        color: "#909090"
        opacity: root.active
        z: -1
        anchors.rightMargin: -6
        anchors.leftMargin: -6
        anchors.bottomMargin: -4
        anchors.topMargin: 10
        anchors.fill: parent
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        anchors.margins: enlargeMouseArea ? -8 : 0
        hoverEnabled: true

        QStyleItem { cursor: "pointinghandcursor"; anchors.fill: parent }

        onEntered: {
            if (!root.active)
                mouseArea.state = "hovered"
            root.entered();
        }
        onExited: {
            mouseArea.state = ""
            root.exited();
        }
        onClicked: {
            root.focus = true;
            root.clicked();
        }

        states: [
            State {
                name: "hovered"
                PropertyChanges {
                    target: root
                    font.underline: true
                }
            }
        ]
    }
}
