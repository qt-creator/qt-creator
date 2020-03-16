/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

import QtQuick 2.12
import QtQuick 2.0
import QtQuick.Controls 2.0

Rectangle {
    property bool toggled: false
    property string tooltip
    property bool toggleBackground: false // show a black background for the toggled state
    property var states: [] // array of 2 state-objects, idx 0: untoggled, idx 1: toggled

    id: root
    color: toggleBackground && toggled ? "#aa000000" : mouseArea.containsMouse ? "#44000000" : "#00000000"
    width: txt.width + 5
    height: 16

    Text {
        id: txt
        color: "#b5b5b5"
        anchors.verticalCenter: parent.verticalCenter
        text: root.states[toggled ? 1 : 0].text
    }

    ToolTip {
        text: tooltip
        visible: mouseArea.containsMouse
        delay: 1000
    }

    MouseArea {
        id: mouseArea
        cursorShape: "PointingHandCursor"
        hoverEnabled: true
        anchors.fill: parent
        onClicked: root.toggled = !root.toggled
    }
}


