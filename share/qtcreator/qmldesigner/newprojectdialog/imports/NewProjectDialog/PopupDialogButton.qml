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

import QtQuick
import QtQuick.Controls
import StudioTheme as StudioTheme

Button {
    id: root

    implicitWidth: Math.max(
                       background ? background.implicitWidth : 0,
                       textItem.implicitWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(
                        background ? background.implicitHeight : 0,
                        textItem.implicitHeight + topPadding + bottomPadding)
    leftPadding: 4
    rightPadding: 4

    background: Rectangle {
        id: background
        implicitWidth: 80
        implicitHeight: StudioTheme.Values.height
        color: StudioTheme.Values.themeControlBackground
        border.color: StudioTheme.Values.themeControlOutline
        anchors.fill: parent
    }

    contentItem: Text {
        id: textItem
        text: root.text
        font.family: StudioTheme.Constants.font.family
        font.pixelSize: DialogValues.defaultPixelSize
        color: StudioTheme.Values.themeTextColor
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        renderType: Text.QtRendering
        anchors.fill: parent
    }

    states: [
        State {
            name: "default"
            when: !root.hovered && !root.checked && !root.pressed

            PropertyChanges {
                target: background
                color: StudioTheme.Values.themeControlBackground
                border.color: StudioTheme.Values.themeControlOutline
            }
            PropertyChanges {
                target: textItem
                color: StudioTheme.Values.themeTextColor
            }
        },
        State {
            name: "hover"
            when: root.hovered && !root.checked && !root.pressed

            PropertyChanges {
                target: background
                color: StudioTheme.Values.themeControlBackgroundHover
                border.color: StudioTheme.Values.themeControlOutline
            }
            PropertyChanges {
                target: textItem
                color: StudioTheme.Values.themeTextColor
            }
        },
        State {
            name: "press"
            when: root.hovered && root.pressed

            PropertyChanges {
                target: background
                color: StudioTheme.Values.themeInteraction
                border.color: StudioTheme.Values.themeInteraction
            }
            PropertyChanges {
                target: textItem
                color: StudioTheme.Values.themeIconColor
            }
        }
    ]
}
