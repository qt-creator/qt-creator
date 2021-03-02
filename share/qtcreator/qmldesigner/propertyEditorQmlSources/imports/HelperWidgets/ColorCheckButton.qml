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
import StudioTheme 1.0 as StudioTheme

Item {
    id: root
    property bool checked: false
    property alias buttonColor: checkBox.color
    width: StudioTheme.Values.height
    height: StudioTheme.Values.height

    signal rightMouseButtonClicked

    Rectangle {
        id: backgroundBox
        width: StudioTheme.Values.height
        height: StudioTheme.Values.height
        anchors.right: parent.right

        color: "white"
        border.color: "white"
        border.width: StudioTheme.Values.border

        Rectangle {
            id: checkBox
            width: StudioTheme.Values.height - 2 * StudioTheme.Values.border
            height: StudioTheme.Values.height - 2 * StudioTheme.Values.border
            anchors.centerIn: parent

            border.color: "black"
            border.width: StudioTheme.Values.border
        }
    }

    Image {
        id: arrowImage
        width: 8
        height: 4
        source: "image://icons/down-arrow"
        anchors.verticalCenter: parent.verticalCenter
        anchors.right: backgroundBox.left
        anchors.rightMargin: 4
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
