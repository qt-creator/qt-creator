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
import QtQuick.Templates as T
import StudioTheme 1.0 as StudioTheme

T.Button {
    id: root

    implicitWidth: Math.max(
                       background ? background.implicitWidth : 0,
                       textItem.implicitWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(
                        background ? background.implicitHeight : 0,
                        textItem.implicitHeight + topPadding + bottomPadding)
    leftPadding: StudioTheme.Values.dialogButtonPadding
    rightPadding: StudioTheme.Values.dialogButtonPadding

    background: Rectangle {
        id: background
        implicitWidth: 70
        implicitHeight: 20
        color: StudioTheme.Values.themeControlBackground
        border.color: StudioTheme.Values.themeControlOutline
        anchors.fill: parent
    }

    contentItem: Text {
        id: textItem
        text: root.text
        font.pixelSize: StudioTheme.Values.baseFontSize
        color: StudioTheme.Values.themeTextColor
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }

    states: [
        State {
            name: "default"
            when: root.enabled && !root.down && !root.hovered && !root.checked

            PropertyChanges {
                target: background
                color: root.highlighted ? StudioTheme.Values.themeInteraction
                                        : StudioTheme.Values.themeControlBackground
                border.color: StudioTheme.Values.themeControlOutline
            }
            PropertyChanges {
                target: textItem
                color: StudioTheme.Values.themeTextColor
            }
        },
        State {
            name: "hover"
            when: root.enabled && root.hovered && !root.checked && !root.down

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
            when: root.enabled && (root.checked || root.down)

            PropertyChanges {
                target: background
                color: StudioTheme.Values.themeControlBackgroundInteraction
                border.color: StudioTheme.Values.themeControlOutlineInteraction
            }
            PropertyChanges {
                target: textItem
                color: StudioTheme.Values.themeTextColor
            }
        },
        State {
            name: "disable"
            when: !root.enabled
            PropertyChanges {
                target: background
                color: StudioTheme.Values.themeControlBackgroundDisabled
                border.color: StudioTheme.Values.themeControlOutlineDisabled
            }
            PropertyChanges {
                target: textItem
                color: StudioTheme.Values.themeTextColorDisabled
            }
        }
    ]
}
