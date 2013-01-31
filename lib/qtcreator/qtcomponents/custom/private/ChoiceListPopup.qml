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

MouseArea {
    id: popup

    // There is no global z-ordering that can stack this popup in front, so we
    // need to reparent it to the root item to fake it upon showing the popup.
    // In that case, the popup will also fill the whole window to allow the user to
    // close the popup by clicking anywhere in the window. Letting the popup act as the mouse
    // area for the button that 'owns' it is also necessary to support drag'n'release behavior.

    // The 'popupframe' delegate will be told to show or hide by assigning
    // opacity to 1 or 0, respectively.

    anchors.fill: parent
    hoverEnabled: true    



    // Set 'popupOpen' to show/hide the popup. The 'state' property is more
    // internal, and contains additional states used to protect the popup from
    // e.g. receiving mouse clicks while its about to hide etc.
    property bool popupOpen: false

    property bool desktopBehavior: true
    property int previousCurrentIndex: -1
    property alias model: listView.model
    property alias currentIndex: listView.currentIndex
    property string currentText: model && currentIndex >= 0 ? model.get(currentIndex).text : ""

    // buttonPressed will be true when the mouse press starts
    // while the popup is closed. At that point, this component can be
    // seen as a button, and not yet a popup menu:
    property bool buttonPressed: false

    property Component listItem
    property Component listHighlight
    property Component popupFrame

    property Item originalParent: parent

    onPopupOpenChanged: {
        if (popupFrameLoader.item === null)
            return;
        if (popupOpen) {
            var oldMouseX = mouseX

            // Reparent to root, so the popup stacks in front:
            originalParent = parent;
            var p = parent;
            while (p.parent != undefined)
                p = p.parent
            parent = p;

            previousCurrentIndex = currentIndex;
            positionPopup();
            popupFrameLoader.item.opacity = 1;
            if (oldMouseX === mouseX){
                // Work around bug: mouseX and mouseY does not immidiatly
                // update after reparenting and resizing the mouse area:
                var pos = originalParent.mapToItem(parent, mouseX, mouseY)
                highlightItemAt(pos.x, pos.y);
            } else {
                highlightItemAt(mouseX, mouseY);
            }
            listView.forceActiveFocus();
            state = "popupOpen"
        } else {
            popupFrameLoader.item.opacity = 0;
            popup.hideHighlight();
            state = "popupClosed"
        }
    }

    Component.onCompleted: {
        // In case 'popupOpen' was set to 'true' before
        // 'popupFrameLoader' was finished, we open the popup now instead:
        if (popup.popupOpen){
            popup.popupOpen = false
            popup.popupOpen = true
        }
    }

    function highlightItemAt(posX, posY)
    {
        var mappedPos = mapToItem(listView.contentItem, posX, posY);
        var indexAt = listView.indexAt(mappedPos.x, mappedPos.y);
        if (indexAt == listView.highlightedIndex)
            return;
        if (indexAt >= 0) {
            listView.highlightedIndex = indexAt;
        } else {
            if(posY > listView.y+listView.height && listView.highlightedIndex+1 < listView.count ) {
                listView.highlightedIndex++;
            } else if(posY < listView.y && listView.highlightedIndex > 0) {
                listView.highlightedIndex--;
            } else if(posX < popupFrameLoader.x || posX > popupFrameLoader.x+popupFrameLoader.width) {
                popup.hideHighlight();
            }
        }
    }

    function hideHighlight() {
        listView.highlightedIndex = -1;
        listView.highlightedItem = null; // will trigger positionHighlight() what will hide the highlight
    }

    function positionPopup() {
        // Set initial values to top left corner of original parent:
        var globalPos = mapFromItem(originalParent, 0, 0);
        var newX = globalPos.x;
        var newY = globalPos.y
        var newW = originalParent.width;
        var newH = listView.contentHeight

        switch (popupFrameLoader.item.popupLocation) {
        case "center":
            // Show centered over original parent with respect to selected item:
            var itemHeight = Math.max(listView.contentHeight/listView.count, 0);
            var currentItemY = Math.max(currentIndex*itemHeight, 0);
            currentItemY += Math.floor(itemHeight/2 - choiceList.height/2);  // correct for choiceLists that are higher than items in the list
            newY -= currentItemY;
            break;
        case "below":
        case "":
            // Show below original parent:
            newX -= popupFrameLoader.anchors.leftMargin;
            newY += originalParent.height - popupFrameLoader.anchors.topMargin;
            break;
        }

        // Ensure the popup is inside the window:
        if (newX < popupFrameLoader.anchors.leftMargin)
            newX = popupFrameLoader.anchors.leftMargin;
        else if (newX + newW > popup.width - popupFrameLoader.anchors.rightMargin)
            newX = popup.width - popupFrameLoader.anchors.rightMargin - newW;

        if (newY < popupFrameLoader.anchors.topMargin)
            newY = popupFrameLoader.anchors.topMargin;
        else if (newY + newH > popup.height - popupFrameLoader.anchors.bottomMargin)
            newY = popup.height - popupFrameLoader.anchors.bottomMargin - newH;

        // Todo: handle case when the list itself is larger than the window...

        listView.x = newX
        listView.y = newY
        listView.width = newW
        listView.height = newH
    }

    Loader {
        id: popupFrameLoader
        property alias styledItem: popup.originalParent
        anchors.fill: listView
        anchors.leftMargin: -item.anchors.leftMargin
        anchors.rightMargin: -item.anchors.rightMargin
        anchors.topMargin: -item.anchors.topMargin
        anchors.bottomMargin: -item.anchors.bottomMargin
        sourceComponent: popupFrame
        onItemChanged: item.opacity = 0
    }

    ListView {
        id: listView
        focus: true
        opacity: popupFrameLoader.item.opacity
        boundsBehavior: desktopBehavior ? ListView.StopAtBounds : ListView.DragOverBounds
        keyNavigationWraps: !desktopBehavior
        highlightFollowsCurrentItem: false  // explicitly handled below

        interactive: !desktopBehavior   // disable flicking. also disables key handling
        onCurrentItemChanged: {
            if(desktopBehavior) {
                positionViewAtIndex(currentIndex, ListView.Contain);
            }
        }

        property int highlightedIndex: -1
        onHighlightedIndexChanged: positionViewAtIndex(highlightedIndex, ListView.Contain)

        property variant highlightedItem: null
        onHighlightedItemChanged: {
            if(desktopBehavior) {
                positionHighlight();
            }
        }

        function positionHighlight() {
            if(!Qt.isQtObject(highlightItem))
                return;

            if(!Qt.isQtObject(highlightedItem)) {
                highlightItem.opacity = 0;  // hide when no item is highlighted
            } else {
                highlightItem.x = highlightedItem.x;
                highlightItem.y = highlightedItem.y;
                highlightItem.width = highlightedItem.width;
                highlightItem.height = highlightedItem.height;
                highlightItem.opacity = 1;  // show once positioned
            }
        }

        delegate: Item {
            id: itemDelegate
            width: delegateLoader.item.width
            height: delegateLoader.item.height
            property int theIndex: index // for some reason the loader can't bind directly to 'index'

            Loader {
                id: delegateLoader
                property variant model: listView.model
                property alias index: itemDelegate.theIndex
                property Item styledItem: choiceList
                property bool highlighted: theIndex == listView.highlightedIndex
                property string itemText: popup.model.get(theIndex).text
                sourceComponent: listItem
            }

            states: State {
                name: "highlighted"
                when: index == listView.highlightedIndex
                StateChangeScript {
                    script: {
                        if(Qt.isQtObject(listView.highlightedItem)) {
                            listView.highlightedItem.yChanged.disconnect(listView.positionHighlight);
                        }
                        listView.highlightedItem = itemDelegate;
                        listView.highlightedItem.yChanged.connect(listView.positionHighlight);
                    }
                }

            }
        }

        function firstVisibleItem() { return indexAt(contentX+10,contentY+10); }
        function lastVisibleItem() { return indexAt(contentX+width-10,contentY+height-10); }
        function itemsPerPage() { return lastVisibleItem() - firstVisibleItem(); }

        Keys.onPressed: {
            // with the ListView !interactive (non-flicking) we have to handle arrow keys
            if (event.key == Qt.Key_Up) {
                if(!highlightedItem) highlightedIndex = lastVisibleItem();
                else if(highlightedIndex > 0) highlightedIndex--;
            } else if (event.key == Qt.Key_Down) {
                if(!highlightedItem) highlightedIndex = firstVisibleItem();
                else if(highlightedIndex+1 < model.count) highlightedIndex++;
            } else if (event.key == Qt.Key_PageUp) {
                if(!highlightedItem) highlightedIndex = lastVisibleItem();
                else highlightedIndex = Math.max(highlightedIndex-itemsPerPage(), 0);
            } else if (event.key == Qt.Key_PageDown) {
                if(!highlightedItem) highlightedIndex = firstVisibleItem();
                else highlightedIndex = Math.min(highlightedIndex+itemsPerPage(), model.count-1);
            } else if (event.key == Qt.Key_Home) {
                highlightedIndex = 0;
            } else if (event.key == Qt.Key_End) {
                highlightedIndex = model.count-1;
            } else if (event.key == Qt.Key_Enter || event.key == Qt.Key_Return) {
                if(highlightedIndex != -1) {
                    listView.currentIndex = highlightedIndex;
                } else {
                    listView.currentIndex = popup.previousCurrentIndex;
                }

                popup.popupOpen = false;
            } else if (event.key == Qt.Key_Escape) {
                listView.currentIndex = popup.previousCurrentIndex;
                popup.popupOpen = false;
            }
            event.accepted = true;  // consume all keys while popout has focus
        }

        highlight: popup.listHighlight
    }

    Timer {
        // This is the time-out value for when we consider the
        // user doing a press'n'release, and not just a click to
        // open the popup:
        id: pressedTimer
        interval: 400 // Todo: fetch value from style object
    }

    onPressed: {
        if (state == "popupClosed") {
            // Show the popup:
            pressedTimer.running = true
            popup.popupOpen = true
            popup.buttonPressed = true
        }
    }

    onReleased: {
        if (state == "popupOpen" && pressedTimer.running === false) {
            // Either we have a 'new' click on the popup, or the user has
            // done a drag'n'release. In either case, the user has done a selection:
            var mappedPos = mapToItem(listView.contentItem, mouseX, mouseY);
            var indexAt = listView.indexAt(mappedPos.x, mappedPos.y);
            if(indexAt != -1)
                listView.currentIndex = indexAt;
            popup.popupOpen = false
        }
        popup.buttonPressed = false
    }

    onPositionChanged: {
        if (state == "popupOpen")
            popup.highlightItemAt(mouseX, mouseY)
    }

    states: [
        State {
            name: "popupClosed"
            when: popupFrameLoader.item.opacity === 0;
            StateChangeScript {
                script: parent = originalParent;
            }
        }
    ]
}




