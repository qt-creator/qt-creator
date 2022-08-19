// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick 2.1
import QtQuick.Controls 1.1

Item {
    id: content

    property Component menuItemDelegate
    property Component scrollerStyle
    property var itemsModel
    property int minWidth: 100
    property real maxHeight: 800
    property int margin: 1

    signal triggered(var item)

    function menuItemAt(index) {
        list.currentIndex = index
        return list.currentItem
    }

    width: Math.max(list.contentWidth, minWidth)
    height: Math.min(list.contentHeight, fittedMaxHeight) + 2 * margin

    readonly property int currentIndex: menu.__currentIndex
    property Item currentItem: null
    readonly property int itemHeight: (list.count > 0 && list.contentItem.children[0]) ? list.contentItem.children[0].height : 23
    readonly property int fittingItems: Math.floor((maxHeight - downScroller.height) / itemHeight)
    readonly property real fittedMaxHeight: itemHeight * fittingItems + downScroller.height
    readonly property bool shouldUseScrollers: scrollView.__style.useScrollers && itemsModel.length > fittingItems
    readonly property real upScrollerHeight: upScroller.visible ? upScroller.height : 0
    readonly property real downScrollerHeight: downScroller.visible ? downScroller.height : 0

    function updateCurrentItem(mouse) {
        var pos = mapToItem(list.contentItem, mouse.x, mouse.y)
        if (!currentItem || !currentItem.contains(Qt.point(pos.x - currentItem.x, pos.y - currentItem.y))) {
            if (currentItem && !hoverArea.pressed && currentItem.isSubmenu)
                currentItem.closeSubMenu()
            currentItem = list.itemAt(pos.x, pos.y)
            if (currentItem) {
                menu.__currentIndex = currentItem.menuItemIndex
                if (currentItem.isSubmenu && !currentItem.menuItem.__popupVisible)
                    currentItem.showSubMenu(false)
            } else {
                menu.__currentIndex = -1
            }
        }
    }

    ScrollView {
        id: scrollView
        anchors {
            fill: parent
            topMargin: content.margin + upScrollerHeight
            bottomMargin: downScrollerHeight - content.margin - 1
            rightMargin: -1
        }

        style: scrollerStyle
        __wheelAreaScrollSpeed: itemHeight

        ListView {
            id: list
            model: itemsModel
            delegate: menuItemDelegate
            snapMode: ListView.SnapToItem
            boundsBehavior: Flickable.StopAtBounds
            highlightFollowsCurrentItem: true
            highlightMoveDuration: 0
        }
    }

    MouseArea {
        id: hoverArea
        anchors.left: scrollView.left
        width: scrollView.width - scrollView.__verticalScrollBar.width
        height: parent.height

        hoverEnabled: true
        acceptedButtons: Qt.AllButtons

        onPositionChanged: updateCurrentItem(mouse)
        onReleased: content.triggered(currentItem)
        onExited: {
            if (currentItem && !currentItem.menuItem.__popupVisible) {
                currentItem = null
                menu.__currentIndex = -1
            }
        }

        MenuContentScroller {
            id: upScroller
            direction: "up"
            visible: shouldUseScrollers && !list.atYBeginning
            x: margin
            function scrollABit() { list.contentY -= itemHeight }
        }

        MenuContentScroller {
            id: downScroller
            direction: "down"
            visible: shouldUseScrollers && !list.atYEnd
            x: margin
            function scrollABit() { list.contentY += itemHeight }
        }
    }

    Timer {
        interval: 1
        running: true
        repeat: false
        onTriggered: list.positionViewAtIndex(currentIndex, scrollView.__style.useScrollers
                                                            ? ListView.Center : ListView.Beginning)
    }

    Binding {
        target: scrollView.__verticalScrollBar
        property: "singleStep"
        value: itemHeight
    }
}
