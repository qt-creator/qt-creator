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

Flickable {
    id: flickable

    property alias horizontalThickness: horizontalScrollBar.height
    property alias verticalThickness: verticalScrollBar.width
    readonly property bool verticalScrollBarVisible: verticalScrollBar.scrollBarVisible
    readonly property bool horizontalScrollBarVisible: horizontalScrollBar.scrollBarVisible
    readonly property bool bothVisible: verticalScrollBarVisible && horizontalScrollBarVisible

    property real temporaryHeight: 0

    contentWidth: areaItem.childrenRect.width
    contentHeight: Math.max(areaItem.childrenRect.height, flickable.temporaryHeight)
    boundsBehavior: Flickable.StopAtBounds

    default property alias content: areaItem.children

    Item {
        id: areaItem
    }

    ScrollBar.horizontal: HorizontalScrollBar {
        id: horizontalScrollBar
        parent: flickable
        scrollBarVisible: flickable.contentWidth > flickable.width
    }

    ScrollBar.vertical: VerticalScrollBar {
        id: verticalScrollBar
        parent: flickable
        scrollBarVisible: flickable.contentHeight > flickable.height
    }
}
