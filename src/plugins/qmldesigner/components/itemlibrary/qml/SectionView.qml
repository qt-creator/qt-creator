/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

Column {
    id: sectionView

    // public
    
    property var itemHighlight

    property int entriesPerRow
    property int cellWidth
    property int cellHeight

    function expand() {
        gridFrame.state = "";
    }

    signal itemSelected(int itemLibId)
    signal itemDragged(int itemLibId)

    // internal

    ItemsViewStyle { id: style }
    
    Component {
        id: itemDelegate

        ItemView {
            id: item

	    width: cellWidth
	    height: cellHeight

            function selectItem() {
                itemHighlight.select(sectionView, item, gridFrame.x, -gridView.viewportY);
                sectionView.itemSelected(itemLibId);
            }

            onItemPressed: selectItem()
            onItemDragged: {
                selectItem();
                sectionView.itemDragged(itemLibId);
            }
        }
    }

    Rectangle {
        width: parent.width
        height: style.sectionTitleHeight

        color: style.sectionTitleBackgroundColor
        radius: 2

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

            text: sectionName
            color: style.sectionTitleTextColor
        }
        MouseArea {
            anchors.fill: parent
            onClicked: {
                if (itemHighlight.visible &&
                    (itemHighlight.section == sectionView)) {
                    itemHighlight.unselect();
                    sectionView.itemSelected(-1);
                }
                gridFrame.toggleExpanded()
            }
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

	    // workaround
            Connections {
                target: itemLibraryModel
                onVisibilityUpdated: gridView.positionViewAtIndex(0);
            }

            anchors.fill: parent
            anchors.rightMargin: 1
            anchors.bottomMargin: 1

            cellWidth: sectionView.cellWidth
            cellHeight: sectionView.cellHeight
            model: sectionEntries
            delegate: itemDelegate
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
