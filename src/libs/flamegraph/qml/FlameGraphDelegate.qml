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
import FlameGraph 1.0

Item {
    id: flamegraphItem
    property color borderColor
    property real borderWidth
    property real itemHeight
    property bool isSelected
    property string text;

    signal mouseEntered
    signal mouseExited
    signal clicked

    property bool textVisible: width > 20 || isSelected
    property int level: parent.level !== undefined ? parent.level + 1 : 0

    height: parent === null ?
                0 : (parent.height - (parent.itemHeight !== undefined ? parent.itemHeight : 0));
    width: parent === null ? 0 : parent.width * FlameGraph.relativeSize
    x: parent === null ? 0 : parent.width * FlameGraph.relativePosition

    Rectangle {
        border.color: borderColor
        border.width: borderWidth
        color: Qt.hsla((level % 12) / 72, 0.9 + Math.random() / 10,
                       0.45 + Math.random() / 10, 0.9 + Math.random() / 10);
        height: itemHeight;
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom

        FlameGraphText {
            id: text
            visible: textVisible
            anchors.fill: parent
            anchors.margins: 5
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignHCenter
            text: flamegraphItem.text
            elide: Text.ElideRight
            wrapMode: Text.WrapAtWordBoundaryOrAnywhere
            font.bold: isSelected
        }

        MouseArea {
            anchors.fill: parent
            hoverEnabled: true

            onEntered: flamegraphItem.mouseEntered()
            onExited: flamegraphItem.mouseExited()
            onClicked: flamegraphItem.clicked()

        }
    }
}
