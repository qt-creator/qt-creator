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
import QtQuick.Layouts 1.15
import HelperWidgets 2.0

MouseArea {
    id: mouseArea

    signal showContextMenu()

    onExited: tooltipBackend.hideTooltip()
    onCanceled: tooltipBackend.hideTooltip()
    onPositionChanged: tooltipBackend.reposition()
    onClicked: function(mouse) {
        forceActiveFocus()
        if (mouse.button === Qt.RightButton)
            showContextMenu()
    }

    hoverEnabled: true
    acceptedButtons: Qt.LeftButton | Qt.RightButton

    Timer {
        interval: 1000
        running: mouseArea.containsMouse
        onTriggered: {
            tooltipBackend.name = itemName
            tooltipBackend.path = componentPath
            tooltipBackend.showTooltip()
        }
    }
}
