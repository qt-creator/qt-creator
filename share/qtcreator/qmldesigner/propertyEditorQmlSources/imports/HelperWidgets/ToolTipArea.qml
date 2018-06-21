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
import QtQuick.Controls 1.1 as Controls
import QtQuick.Layouts 1.0
import QtQuick.Controls.Private 1.0

MouseArea {
    id: mouseArea

    onExited: Tooltip.hideText()
    onCanceled: Tooltip.hideText()

    hoverEnabled: true

    property string tooltip

    Timer {
        interval: 1000
        running: mouseArea.containsMouse && tooltip.length
        onTriggered: Tooltip.showText(mouseArea, Qt.point(mouseArea.mouseX, mouseArea.mouseY), tooltip)
    }
}
