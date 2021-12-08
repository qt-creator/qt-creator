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

import QtQuick 2.9
import welcome 1.0
import StudioFonts 1.0

Image {
    id: account_icon

    source: "images/" + (mouseArea.containsMouse ? "icon_hover.png" : "icon_default.png")

    Text {
        id: account
        color: mouseArea.containsMouse ? Constants.textHoverColor
                                       : Constants.textDefaultColor
        text: qsTr("Account")
        anchors.top: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        font.family: StudioFonts.titilliumWeb_regular
        font.pixelSize: 16
        renderType: Text.NativeRendering
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        anchors.margins: -25
        hoverEnabled: true

        onClicked: Qt.openUrlExternally("https://login.qt.io/login/")
    }
}
