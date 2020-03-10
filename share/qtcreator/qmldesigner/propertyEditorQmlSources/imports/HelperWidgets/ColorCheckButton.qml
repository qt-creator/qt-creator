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

import QtQuick 2.1

Item {
    id: root
    property bool checked: false
    property alias buttonColor: checkBox.color
    width: 30
    height: 24

    signal rightMouseButtonClicked

    Rectangle {
        id: backgroundBox
        width: 24
        height: 24
        anchors.right: parent.right

        color: "white"
        border.color: "white"
        border.width: 1

        Rectangle {
            id: checkBox
            width: 22
            height: 22
            anchors.centerIn: parent

            border.color: "black"
            border.width: 1
        }
    }

    Image {
        id: arrowImage
        width: 8
        height: 4
        source: "image://icons/down-arrow"
        anchors.verticalCenter: parent.verticalCenter
        anchors.right: backgroundBox.left
        anchors.rightMargin: 2
        opacity: colorToolTip.containsMouse ? 1 : 0.8
        rotation: root.checked ? 0.0 : 270.0
    }

    ToolTipArea {
        id: colorToolTip

        acceptedButtons: Qt.LeftButton | Qt.RightButton

        onClicked: {
            if (mouse.button === Qt.RightButton)
                root.rightMouseButtonClicked()
            else
                root.checked = !root.checked
        }
        hoverEnabled: true
        anchors.fill: parent
        anchors.leftMargin: -arrowImage.width
        tooltip: qsTr("Toggle color picker view.")
    }
}
