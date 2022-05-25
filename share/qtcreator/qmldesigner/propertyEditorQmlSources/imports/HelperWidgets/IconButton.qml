/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuickDesignerTheme 1.0
import StudioTheme 1.0 as StudioTheme

Rectangle {
    id: root

    signal clicked()

    property alias icon: icon.text
    property alias enabled: mouseArea.enabled
    property alias tooltip: toolTip.text
    property alias iconSize: icon.font.pixelSize

    property int buttonSize: StudioTheme.Values.height
    property color normalColor: StudioTheme.Values.themeControlBackground
    property color hoverColor: StudioTheme.Values.themeControlBackgroundHover
    property color pressColor: StudioTheme.Values.themeControlBackgroundInteraction

    width: buttonSize
    height: buttonSize

    color: mouseArea.pressed ? pressColor
                             : mouseArea.containsMouse ? hoverColor
                                                       : normalColor

    Behavior on color {
        ColorAnimation {
            duration: 300
            easing.type: Easing.OutQuad
        }
    }

    Text {
        id: icon

        color: root.enabled ? StudioTheme.Values.themeTextColor : StudioTheme.Values.themeTextColorDisabled
        font.family: StudioTheme.Constants.iconFont.family
        font.pixelSize: StudioTheme.Values.baseIconFontSize
        anchors.centerIn: root
    }

    MouseArea {
        id: mouseArea

        anchors.fill: parent
        hoverEnabled: true
        onClicked: root.clicked()
    }

    ToolTip {
        id: toolTip

        visible: mouseArea.containsMouse
        delay: 1000
    }
}
