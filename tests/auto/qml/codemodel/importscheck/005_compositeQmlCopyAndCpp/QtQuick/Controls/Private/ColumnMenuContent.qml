/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt Quick Controls module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

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
