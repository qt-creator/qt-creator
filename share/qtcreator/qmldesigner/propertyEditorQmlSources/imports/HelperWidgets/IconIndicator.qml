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
import HelperWidgets 2.0
import StudioTheme 1.0 as StudioTheme

Rectangle {
    id: root

    property Item myControl
    property alias icon: indicatorIcon.text
    property alias iconColor: indicatorIcon.color
    property alias pixelSize: indicatorIcon.font.pixelSize
    property alias tooltip: toolTipArea.tooltip

    property bool hovered: toolTipArea.containsMouse && root.enabled

    signal clicked()

    color: "transparent"
    border.color: "transparent"

    implicitWidth: StudioTheme.Values.linkControlWidth
    implicitHeight: StudioTheme.Values.linkControlHeight

    z: 10

    T.Label {
        id: indicatorIcon
        anchors.fill: parent
        text: "?"
        visible: true
        color: StudioTheme.Values.themeTextColor
        font.family: StudioTheme.Constants.iconFont.family
        font.pixelSize: StudioTheme.Values.myIconFontSize
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter
    }

    ToolTipArea {
        id: toolTipArea
        anchors.fill: parent
        onClicked: root.clicked()
    }

    states: [
        State {
            name: "default"
            when: !toolTipArea.containsMouse && root.enabled
            PropertyChanges {
                target: indicatorIcon
                color: StudioTheme.Values.themeLinkIndicatorColor
            }
        },
        State {
            name: "hover"
            when: toolTipArea.containsMouse && root.enabled
            PropertyChanges {
                target: indicatorIcon
                color: StudioTheme.Values.themeLinkIndicatorColorHover
            }
        },
        State {
            name: "disable"
            when: !root.enabled
            PropertyChanges {
                target: indicatorIcon
                color: StudioTheme.Values.themeLinkIndicatorColorDisabled
            }
        }
    ]
}
