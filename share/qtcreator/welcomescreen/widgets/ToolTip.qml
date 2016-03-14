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
    id: toolTip

    property alias text: text.text
    property int margin: 4

    width: text.width + margin * 2
    height: text.height + margin * 2

    opacity: 0

    property Item originalParent: parent

    property int oldX: x
    property int oldY: y

    Behavior on opacity {
        SequentialAnimation {
            PauseAnimation { duration: opacity === 0 ? 1000 : 0 }
            PropertyAnimation { duration: 50 }
        }
    }

    function show() {
        toolTip.originalParent = toolTip.parent;
        var p = toolTip.parent;
        while (p.parent != undefined && p.parent.parent != undefined)
            p = p.parent
        toolTip.parent = p;

        toolTip.oldX = toolTip.x
        toolTip.oldY = toolTip.y
        var globalPos = mapFromItem(toolTip.originalParent, toolTip.oldX, toolTip.oldY);

        toolTip.x = globalPos.x + toolTip.oldX
        toolTip.y = globalPos.y + toolTip.oldY

        toolTip.opacity = 1;
    }

    function hide() {
        toolTip.opacity = 0;
        var oldClip = originalParent.clip
        originalParent.clip = false
        toolTip.parent = originalParent
        originalParent.clip = true
        originalParent.clip = oldClip
        toolTip.x = toolTip.oldX
        toolTip.y = toolTip.oldY
    }

    Rectangle {
        anchors.fill: parent
        color: creatorTheme.Welcome_BackgroundColor
        border.width: 1
        border.color: creatorTheme.Welcome_ForegroundSecondaryColor
    }

    NativeText {
        x: toolTip.margin
        y: toolTip.margin
        id: text
        color: creatorTheme.Welcome_TextColor
    }
}
