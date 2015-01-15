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
** a written agreement between you and The Qt Company. For licensing terms and
** conditions see http://www.qt.io/terms-conditions. For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

import QtQuick 2.0
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2

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
            border.color: "#c8c8c8"
            color: "#eaeaea"
        }
        label: Text {
            text: button.labelText
            textFormat: Text.PlainText
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignLeft
            elide: Text.ElideRight
            renderType: Text.NativeRendering
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

