// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
