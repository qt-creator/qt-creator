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

import QtQuick 2.12
import QtQuick.Templates 2.12 as T
import StudioTheme 1.0 as StudioTheme

T.ScrollBar {
    id: control

    // This needs to be set, when using T.ScrollBar
    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    active: true
    interactive: true
    visible: control.size < 1.0 || T.ScrollBar.AlwaysOn

    snapMode: T.ScrollBar.SnapAlways // TODO
    policy: T.ScrollBar.AsNeeded

    padding: 1 // TODO 0
    size: 1.0
    position: 1.0
    //orientation: Qt.Vertical

    contentItem: Rectangle {
        id: controlHandle
        implicitWidth: 4
        implicitHeight: 4
        radius: width / 2 // TODO 0
        color: StudioTheme.Values.themeScrollBarHandle
    }

    background: Rectangle {
        id: controlTrack
        color: StudioTheme.Values.themeScrollBarTrack
    }
}
