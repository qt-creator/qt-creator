/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

import Qt 4.6

// view displaying one item library section including its grid

Column {
    id: sectionView

    // public

    property var itemHighlight

    property int entriesPerRow
    property int cellWidth
    property int cellHeight

    property var currentItem: gridView.currentItem

    function expand() {
        gridFrame.state = ""
    }

    signal itemSelected(int itemLibId)
    signal itemDragged(int itemLibId)

    function setSelection(itemSectionIndex)
    {
        gridView.currentIndex = itemSectionIndex
    }

    function unsetSelection()
    {
        gridView.currentIndex = -1
    }

    function focusSelection(flickable) {
        var pos = -1;

        if (!gridView.currentItem)
            return;

        var currentItemX = sectionView.x + gridFrame.x + gridView.x + gridView.currentItem.x;
        var currentItemY = sectionView.y + gridFrame.y + gridView.y + gridView.currentItem.y
        - gridView.contentY;  // workaround: GridView reports wrong contentY

        if (currentItemY < flickable.contentY)
            pos = Math.max(0, currentItemY)

            else if ((currentItemY + gridView.currentItem.height) >
                     (flickable.contentY + flickable.height - 1))
                pos = Math.min(Math.max(0, flickable.contentHeight - flickable.height),
        currentItemY + gridView.currentItem.height - flickable.height + 1)

                if (pos >= 0)
                    flickable.contentY = pos
    }

    // internal

    ItemsViewStyle { id: style }

    Component {
        id: itemDelegate

        ItemView {
            id: item

            width: cellWidth
            height: cellHeight

            onItemPressed: sectionView.itemSelected(itemLibId)
            onItemDragged: sectionView.itemDragged(itemLibId)
        }
    }

    // clickable header bar
    Rectangle {
        width: parent.width
        height: style.sectionTitleHeight

        color: style.sectionTitleBackgroundColor

        Item {
            id: arrow

            Rectangle { y: 0; x: 0; height: 1; width: 9; color: style.sectionArrowColor }
            Rectangle { y: 1; x: 1; height: 1; width: 7; color: style.sectionArrowColor }
            Rectangle { y: 2; x: 2; height: 1; width: 5; color: style.sectionArrowColor }
            Rectangle { y: 3; x: 3; height: 1; width: 3; color: style.sectionArrowColor }
            Rectangle { y: 4; x: 4; height: 1; width: 1; color: style.sectionArrowColor }

            anchors.left: parent.left
            anchors.leftMargin: 5
            anchors.verticalCenter: parent.verticalCenter
            width: 9
            height: 5

            transformOrigin: Item.Center
        }
        Text {
            id: text

            anchors.verticalCenter: parent.verticalCenter
            anchors.left: arrow.right
            anchors.leftMargin: 5

            text: sectionName  // to be set by model
            color: style.sectionTitleTextColor
            elide: Text.ElideMiddle
        }
        MouseArea {
            anchors.fill: parent
            onClicked: gridFrame.toggleExpanded()
        }
    }

    Item { height: 2; width: 1 }

    Item {
        id: gridFrame

        function toggleExpanded() {
            state = ((state == "")? "shrunk":"")
        }

        clip: true
        width: entriesPerRow * cellWidth + 1
        height: Math.ceil(sectionEntries.count / entriesPerRow) * cellHeight + 1
        anchors.horizontalCenter: parent.horizontalCenter

        GridView {
            id: gridView

            Connections {
                target: itemLibraryModel  // to be set in Qml context
                onSectionVisibilityChanged: {
                    /* workaround: reset model in order to get the grid view
updated properly under all conditions */
                    if (changedSectionLibId == sectionLibId)
                        gridView.model = sectionEntries
                }
            }

            anchors.fill: parent
            anchors.rightMargin: 1
            anchors.bottomMargin: 1

            cellWidth: sectionView.cellWidth
            cellHeight: sectionView.cellHeight
            model: sectionEntries  // to be set by model
            delegate: itemDelegate
            highlight: itemHighlight
            interactive: false
            highlightFollowsCurrentItem: false
        }

        states: [
        State {
            name: "shrunk"
            PropertyChanges {
                target: gridFrame
                height: 0
                opacity: 0
            }
            PropertyChanges {
                target: arrow
                rotation: -90
            }
        }
        ]
    }

    Item { height: 4; width: 1 }
}
