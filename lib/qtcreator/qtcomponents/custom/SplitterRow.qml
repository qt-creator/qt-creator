/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

import QtQuick 1.0
import "private"

/*
*
* SplitterRow
*
* SplitterRow is a component that provides a way to layout items horisontally with
* a draggable splitter added in-between each item.
*
* Add items to the SplitterRow by inserting them as child items. The splitter handle
* is outsourced as a delegate (handleBackground). For this delegate to work properly,
* it will need to contain a mouse area that communicates with the SplitterRow by binding
* 'onMouseXChanged: handleDragged(handleIndex)', and 'drag.target: dragTarget'.
*
* The SplitterRow contains the followin API:
*
* Component handleBackground - delegate that will be instanciated between each
*   child item. Inside the delegate, the following properties are available:
*   int handleIndex - specifies the index of the splitter handle. The handle
*       between the first and the second item will get index 0, the next handle index 1 etc.
*   Item handleDragTarget - convenience property that tells which drag target any
*       inner mouse areas that controls the handle should bind to.
*   function handleDragged(handleIndex) - function that should be called whenever
*       the handle is dragged to a new position
*
* The following properties can optionally be added for each direct child item of SplitterRow:
*
* real minimumWidth - if present, ensures that the item cannot be resized below the
*   given value. A value of -1 will disable it.
* real maximumWidth - if present, ensures that the item cannot be resized above the
*   given value. A value of -1 will disable it.
* real percentageWidth - if present, should be a value between 0-100. This value specifies
*   a percentage of the width of the SplitterRow width. If the width of the SplitterRow
*   change, the width of the item will change as well. 'percentageWidth' have precedence
*   over 'width', which means that SplitterRow will ignore any assignments done to 'width'.
*   A value of -1 disables it.
* bool expanding - if present, the item will consume all extra space in the SplitterRow, down to
*   minimumWidth. This means that that 'width', 'percentageWidth' and 'maximumWidth' will be ignored.
*   There will always be one (and only one) item in the SplitterRow that has this behaviour, and by
*   default, it will be the last child item of the SplitterRow. Also note that which item that gets
*   resized upon dragging a handle depends on whether the expanding item is located towards the left
*   or the right of the handle.
*
* Example:
*
* To create a SplitterRow with three items, and let
* the center item be the one that should be expanding, one
* could do the following:
*
*    SplitterRow {
*        anchors.fill: parent
*
*        handleBackground: Rectangle {
*            width: 1
*            color: "black"
*
*            MouseArea {
*                anchors.fill: parent
*                anchors.leftMargin: -2
*                anchors.rightMargin: -2
*                drag.axis: Qt.YAxis
*                drag.target: handleDragTarget
*                onMouseXChanged: handleDragged(handleIndex)
*            }
*        }
*
*        Rectangle {
*            color: "gray"
*            width: 200
*        }
*        Rectangle {
*            property real minimumWidth: 50
*            property real maximumWidth: 400
*            property bool expanding: true
*            color: "darkgray"
*        }
*        Rectangle {
*            color: "gray"
*            width: 200
*        }
*    }
*/

Item {
    id: root
    default property alias items: splitterItems.children
    property alias handles: splitterHandles.children
    property Component handleBackground: Rectangle { width:3; color: "black" }
    clip: true

    Component.onCompleted: d.init();
    onWidthChanged: d.updateLayout();

    QtObject {
        id: d
        property int expandingIndex: items.length-1
        property bool updateOptimizationBlock: true
        property bool bindingRecursionGuard: false

        function init()
        {
            for (var i=0; i<items.length-1; ++i) {
                // Anchor each item to fill out all space vertically:
                var item = items[i];
                item.anchors.top = splitterItems.top
                item.anchors.bottom = splitterItems.bottom
                // Listen for changes to width and expanding:
                propertyChangeListener.createObject(item);
                // Create a handle for the item:
                var handle = handleBackgroundLoader.createObject(splitterHandles, {"handleIndex":i});
                handle.anchors.top = splitterHandles.top
                handle.anchors.bottom = splitterHandles.bottom
            }
            item = items[i]
            if (item) {
                // Do the same for the last item as well, since
                // the for-loop skipped the last item:
                items[i].anchors.top = splitterItems.top
                items[i].anchors.bottom = splitterItems.bottom
                propertyChangeListener.createObject(items[i]);
            }
            d.updateOptimizationBlock = false
            d.updateLayout()
        }

        function accumulatedWidth(firstIndex, lastIndex, includeExpandingMinimum)
        {
            // Go through items and handles, and
            // calculate their acummulated width.
            var w = 0
            for (var i=firstIndex; i<lastIndex; ++i) {
                var item = items[i]
                if (i !== d.expandingIndex)
                    w += item.width;
                else if (includeExpandingMinimum && item.minimumWidth != undefined && item.minimumWidth != -1)
                    w += item.minimumWidth
                if (handles[i] && (i !== d.expandingIndex || includeExpandingMinimum === false))
                    w += handles[i].width
            }
            return w
        }

        function updateLayout()
        {
            if (items.length === 0)
                return;
            if (d.updateOptimizationBlock === true)
                return
            d.updateOptimizationBlock = true

            // This function will reposition both handles and
            // items according to the _width of the each item_
            var item, prevItem
            var handle, prevHandle
            var newValue

            // Ensure all items within min/max:
            for (var i=0; i<items.length; ++i) {
                if (i !== d.expandingIndex) {
                    item = items[i];
                    // If the item is using percentage width, convert
                    // that number to real width now:
                    if (item.percentageWidth != undefined && item.percentageWidth !== -1) {
                        newValue = item.percentageWidth * (root.width / 100)
                        if (newValue !== item.width)
                            item.width = newValue
                    }
                    // Ensure item width is not more than maximumWidth:
                    if (item.maximumWidth != undefined && item.maximumWidth != -1) {
                        newValue = Math.min(item.width, item.maximumWidth)
                        if (newValue !== item.width)
                            item.width = newValue
                    }
                    // Ensure item width is not more less minimumWidth:
                    if (item.minimumWidth != undefined && item.minimumWidth != -1) {
                        newValue = Math.max(item.width, item.minimumWidth)
                        if (newValue !== item.width)
                            item.width = newValue
                    }
                }
            }

            // Special case: set width of expanding item to available space:
            newValue = root.width - d.accumulatedWidth(0, items.length, false);
            var expandingItem = items[d.expandingIndex]
            if (expandingItem.minimumWidth != undefined && expandingItem.minimumWidth != -1)
                newValue = Math.max(newValue, expandingItem.minimumWidth)
            if (expandingItem.width !== newValue)
                expandingItem.width = newValue

            // Then, position items and handles according to their width:
            for (i=0; i<items.length; ++i) {
                item = items[i];
                handle = handles[i]

                // Position item to the right of the previus handle:
                if (prevHandle) {
                    newValue = prevHandle.x + prevHandle.width
                    if (newValue !== item.x)
                        item.x = newValue
                }

                // Position handle to the right of item:
                if (handle) {
                    newValue = item.x + Math.max(0, item.width)
                    if (newValue !== handle.x)
                        handle.x = newValue
                }

                prevItem = item
                prevHandle = handle
            }

            d.updateOptimizationBlock = false
        }
    }

    Component {
        id: handleBackgroundLoader
        Loader {
            id: loader
            property int handleIndex: 0
            property Item handleDragTarget: loader
            sourceComponent: handleBackground

            function handleDragged(handleIndex)
            {
                // Moving the handle means resizing an item. Which one,
                // left or right, depends on where the expanding item is.
                // 'updateLayout' will override in case new width violates max/min.
                // And 'updateLayout will be triggered when an item changes width.

                var leftHandle, leftItem, handle, rightItem, rightHandle
                var leftEdge, rightEdge, newWidth

                handle = handles[handleIndex]

                if (d.expandingIndex > handleIndex) {
                    // Resize item to the left.
                    // Ensure that the handle is not crossing other handles:
                    leftHandle = handles[handleIndex-1]
                    leftItem = items[handleIndex]
                    leftEdge = leftHandle ? (leftHandle.x + leftHandle.width) : 0
                    handle.x = Math.max(leftEdge, handle.x)
                    newWidth = handle.x - leftEdge
                    if (root.width != 0 && leftItem.percentageWidth != undefined && leftItem.percentageWidth !== -1)
                        leftItem.percentageWidth = newWidth * (100 / root.width)
                    // The next line will trigger 'updateLayout' inside 'propertyChangeListener':
                    leftItem.width = newWidth
                } else {
                    // Resize item to the right:
                    // Since the first item in the splitter always will have x=0, we need
                    // to ensure that the user cannot drag the handle more left than what
                    // we got space for:
                    var min = d.accumulatedWidth(0, handleIndex+1, true)
                    // Ensure that the handle is not crossing other handles:
                    rightItem = items[handleIndex+1]
                    rightHandle = handles[handleIndex+1]
                    rightEdge = (rightHandle ? rightHandle.x : root.width)
                    handle.x = Math.max(min, Math.max(Math.min((rightEdge - handle.width), handle.x)))
                    newWidth = rightEdge - (handle.x + handle.width)
                    if (root.width != 0 && rightItem.percentageWidth != undefined && rightItem.percentageWidth !== -1)
                        rightItem.percentageWidth = newWidth * (100 / root.width)
                    // The next line will trigger 'updateLayout' inside 'propertyChangeListener':
                    rightItem.width = newWidth
                }
            }
        }
    }

    Item {
        id: splitterItems
        anchors.fill: parent
    }
    Item {
        id: splitterHandles
        anchors.fill: parent
    }

    Component {
        // This dummy item becomes a child of all
        // items it the splitter, just to provide a way
        // to listed for changes to their width, expanding etc.
        id: propertyChangeListener
        Item {
            id: target
            width: parent.width
            property bool expanding: (parent.expanding != undefined) ? parent.expanding : false
            property real percentageWidth: (parent.percentageWidth != undefined) ? parent.percentageWidth : -1
            property real minimumWidth: (parent.minimumWidth != undefined) ? parent.minimumWidth : -1
            property real maximumWidth: (parent.maximumWidth != undefined) ? parent.maximumWidth : -1

            onPercentageWidthChanged: d.updateLayout();
            onMinimumWidthChanged: d.updateLayout();
            onMaximumWidthChanged: d.updateLayout();

            onExpandingChanged: {
                // Find out which item that has the expanding flag:
                for (var i=0; i<items.length; ++i) {
                    var item = items[i]
                    if (item.expanding && item.expanding === true) {
                        d.expandingIndex = i
                        d.updateLayout();
                        return
                    }
                }
                d.expandingIndex = i-1
                updateLayout();
            }

            onWidthChanged: {
                // We need to update the layout:
                if (d.bindingRecursionGuard === true)
                    return
                d.bindingRecursionGuard = true

                // Break binding:
                width = 0
                d.updateLayout()
                // Restablish binding:
                width = function() { return parent.width; }
                
                d.bindingRecursionGuard = false
            }
        }
    }
}
