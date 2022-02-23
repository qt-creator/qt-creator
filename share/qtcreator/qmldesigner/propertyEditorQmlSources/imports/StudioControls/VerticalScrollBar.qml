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
import QtQuick.Controls 2.15
import StudioTheme 1.0 as StudioTheme

ScrollBar {
    id: scrollBar

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    property bool scrollBarVisible: parent.contentHeight > scrollBar.height

    minimumSize: scrollBar.width / scrollBar.height
    orientation: Qt.Vertical
    policy: scrollBar.scrollBarVisible ? ScrollBar.AlwaysOn : ScrollBar.AlwaysOff

    height: parent.availableHeight
            - (parent.bothVisible ? parent.horizontalThickness : 0)
    padding: scrollBar.active ? StudioTheme.Values.scrollBarActivePadding
                              : StudioTheme.Values.scrollBarInactivePadding

    background: Rectangle {
        implicitWidth: StudioTheme.Values.scrollBarThickness
        implicitHeight: StudioTheme.Values.scrollBarThickness
        color: StudioTheme.Values.themeScrollBarTrack
    }

    contentItem: Rectangle {
        implicitWidth: StudioTheme.Values.scrollBarThickness - 2 * scrollBar.padding
        implicitHeight: StudioTheme.Values.scrollBarThickness - 2 * scrollBar.padding
        color: StudioTheme.Values.themeScrollBarHandle
    }
}
