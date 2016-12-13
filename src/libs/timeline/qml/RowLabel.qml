/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

import QtQuick 2.0
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2
import TimelineTheme 1.0

Button {
    id: button
    property var label

    readonly property int dragHeight: 5

    signal selectBySelectionId()
    signal setRowHeight(int newHeight)

    property string labelText: label.description ? label.description : qsTr("[unknown]")
    action: Action {
        onTriggered: button.selectBySelectionId();
        tooltip: button.labelText + (label.displayName ? (" (" + label.displayName + ")") : "")
    }

    style: ButtonStyle {
        background: Rectangle {
            border.width: 1
            border.color: Theme.color(Theme.Timeline_DividerColor)
            color: Theme.color(Theme.PanelStatusBarBackgroundColor)
        }
        label: TimelineText {
            text: button.labelText
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignLeft
            elide: Text.ElideRight
            color: Theme.color(Theme.PanelTextColorLight)
        }
    }
    MouseArea {
        hoverEnabled: true
        property bool resizing: false
        onPressed: resizing = true
        onReleased: resizing = false

        height: dragHeight
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        cursorShape: Qt.SizeVerCursor

        onMouseYChanged: {
            if (resizing)
                button.setRowHeight(y + mouseY)
        }
    }
}

