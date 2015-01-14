/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

import QtQuick 2.1

NativeText {
    id: root
    height: 16
    color: active ? creatorTheme.Welcome_Link_TextColorActive
                  : creatorTheme.Welcome_Link_TextColorNormal
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

    Rectangle {
        color: "#909090" // FIXME: theming: Where is this ever visible?
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
        color: "#909090" // FIXME: theming: Where is this ever visible?
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

        cursorShape: Qt.PointingHandCursor
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
    Accessible.role: Accessible.Link
}
