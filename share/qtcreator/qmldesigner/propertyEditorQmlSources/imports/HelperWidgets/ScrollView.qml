// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import StudioControls as StudioControls
import StudioTheme 1.0 as StudioTheme

Flickable {
    id: flickable

    property alias horizontalThickness: horizontalScrollBar.height
    property alias verticalThickness: verticalScrollBar.width
    readonly property bool verticalScrollBarVisible: verticalScrollBar.scrollBarVisible
    readonly property bool horizontalScrollBarVisible: horizontalScrollBar.scrollBarVisible
    readonly property bool bothVisible: flickable.verticalScrollBarVisible
                                        && flickable.horizontalScrollBarVisible

    property bool hideVerticalScrollBar: false
    property bool hideHorizontalScrollBar: false

    property real temporaryHeight: 0

    default property alias content: areaItem.children

    property bool adsFocus: false
    // objectName is used by the dock widget to find this particular ScrollView
    // and set the ads focus on it.
    objectName: "__mainSrollView"

    flickDeceleration: 10000

    HoverHandler { id: hoverHandler }

    ScrollBar.horizontal: StudioControls.TransientScrollBar {
        id: horizontalScrollBar
        style: StudioTheme.Values.viewStyle
        parent: flickable
        x: 0
        y: flickable.height - horizontalScrollBar.height
        width: flickable.availableWidth - (verticalScrollBar.isNeeded ? verticalScrollBar.thickness : 0)
        orientation: Qt.Horizontal

        visible: !flickable.hideHorizontalScrollBar

        show: (hoverHandler.hovered || flickable.focus || flickable.adsFocus
               || horizontalScrollBar.inUse || horizontalScrollBar.otherInUse)
              && horizontalScrollBar.isNeeded
        otherInUse: verticalScrollBar.inUse
    }

    ScrollBar.vertical: StudioControls.TransientScrollBar {
        id: verticalScrollBar
        style: StudioTheme.Values.viewStyle
        parent: flickable
        x: flickable.width - verticalScrollBar.width
        y: 0
        height: flickable.availableHeight - (horizontalScrollBar.isNeeded ? horizontalScrollBar.thickness : 0)
        orientation: Qt.Vertical

        visible: !flickable.hideVerticalScrollBar

        show: (hoverHandler.hovered || flickable.focus || flickable.adsFocus
               || horizontalScrollBar.inUse || horizontalScrollBar.otherInUse)
              && verticalScrollBar.isNeeded
        otherInUse: horizontalScrollBar.inUse
    }

    contentWidth: areaItem.childrenRect.width
    contentHeight: Math.max(areaItem.childrenRect.height, flickable.temporaryHeight)

    boundsMovement: Flickable.StopAtBounds
    boundsBehavior: Flickable.StopAtBounds

    Item { id: areaItem }

}
