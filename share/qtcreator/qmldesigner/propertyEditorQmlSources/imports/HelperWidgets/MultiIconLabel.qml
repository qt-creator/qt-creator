/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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
import QtQuick.Templates 2.15 as T
import StudioTheme 1.0 as StudioTheme

Rectangle {
    id: root
    property alias tooltip: toolTipArea.tooltip
    property alias icon0: icon0.text
    property alias icon1: icon1.text
    // TODO second alias
    property alias iconColor: icon1.color

    implicitWidth: StudioTheme.Values.controlLabelWidth
    implicitHeight: StudioTheme.Values.controlLabelWidth

    color: "transparent"
    border.color: "transparent"

    T.Label {
        id: icon0
        anchors.fill: parent
        text: ""
        color: Qt.rgba(icon1.color.r,
                       icon1.color.g,
                       icon1.color.b,
                       0.5)
        font.family: StudioTheme.Constants.iconFont.family
        font.pixelSize: StudioTheme.Values.myIconFontSize
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter
    }

    T.Label {
        id: icon1
        anchors.fill: parent
        text: ""
        color: StudioTheme.Values.themeIconColor
        font.family: StudioTheme.Constants.iconFont.family
        font.pixelSize: StudioTheme.Values.myIconFontSize
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter
    }

    ToolTipArea {
        id: toolTipArea
        anchors.fill: parent
    }

    states: [
        State {
            name: "disabled"
            when: !root.enabled
            PropertyChanges {
                target: icon1
                color: StudioTheme.Values.themeIconColorDisabled
            }
        }
    ]
}
