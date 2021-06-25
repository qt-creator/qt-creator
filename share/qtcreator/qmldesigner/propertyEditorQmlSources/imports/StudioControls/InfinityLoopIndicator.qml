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
    id: infinityLoopIndicator

    property Item myControl

    property bool infinite: false

    color: "transparent"
    border.color: "transparent"

    implicitWidth: StudioTheme.Values.infinityControlWidth
    implicitHeight: StudioTheme.Values.infinityControlHeight

    z: 10

    T.Label {
        id: infinityLoopIndicatorIcon
        anchors.fill: parent
        text: StudioTheme.Constants.infinity
        visible: true
        color: StudioTheme.Values.themeTextColor
        font.family: StudioTheme.Constants.iconFont.family
        font.pixelSize: StudioTheme.Values.myIconFontSize
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        onClicked: infinityLoopIndicator.infinite = !infinityLoopIndicator.infinite
    }

    states: [
        State {
            name: "active"
            when: infinityLoopIndicator.infinite && !mouseArea.containsMouse
            PropertyChanges {
                target: infinityLoopIndicatorIcon
                color: StudioTheme.Values.themeInfiniteLoopIndicatorColorInteraction
            }
        },
        State {
            name: "default"
            when: !mouseArea.containsMouse
            PropertyChanges {
                target: infinityLoopIndicatorIcon
                color: StudioTheme.Values.themeInfiniteLoopIndicatorColor
            }
        },
        State {
            name: "hover"
            when: mouseArea.containsMouse
            PropertyChanges {
                target: infinityLoopIndicatorIcon
                color: StudioTheme.Values.themeInfiniteLoopIndicatorColorHover
            }
        }
    ]
}
