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

    property var style
    property var itemHighlight

    property int entriesPerRow
    property int cellWidth
    property int cellHeight

    signal itemSelected(int itemLibId)
    signal itemDragged(int itemLibId)

    function expand() {
        gridFrame.state = "";
    }

    Component {
        id: itemDelegate

        ItemView {
            id: item
            style: sectionView.style

            function selectItem() {
                itemHighlight.select(sectionView, item, gridFrame.x, -gridView.viewportY);
                sectionView.itemSelected(itemLibId);
            }

            onItemClicked: selectItem()
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

            Rectangle { y: 0; x: 0; height: 1; width: 9; color: "#aeaeae" }
            Rectangle { y: 1; x: 1; height: 1; width: 7; color: "#aeaeae" }
            Rectangle { y: 2; x: 2; height: 1; width: 5; color: "#aeaeae" }
            Rectangle { y: 3; x: 3; height: 1; width: 3; color: "#aeaeae" }
            Rectangle { y: 4; x: 4; height: 1; width: 1; color: "#aeaeae" }

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
            Component.onCompleted: text.color = style.sectionTitleTextColor
        }
        MouseRegion {
            anchors.fill: parent
            onClicked: {
                if (itemHighlight.visible &&
                    (itemHighlight.section == sectionView)) {
                    itemHighlight.unselect();
                    sectionView.itemSelected(-1);
                }
                gridFrame.toggleVisibility()
            }
        }
    }

    Item { height: 2; width: 1 }

    Item {
        id: gridFrame

        function toggleVisibility() {
            state = ((state == "hidden")? "":"hidden")
        }

        clip: true
        width: sectionView.entriesPerRow * sectionView.cellWidth + 1
        height: Math.ceil(sectionEntries.count / sectionView.entriesPerRow) * sectionView.cellHeight + 1
        anchors.horizontalCenter: parent.horizontalCenter

        GridView {
            id: gridView

            Connection {
                sender: itemLibraryModel
                signal: "visibilityUpdated()"
                script: gridView.positionViewAtIndex(0)
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
                name: "hidden"
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
/*
        transitions: [
            Transition {
                NumberAnimation {
                    matchProperties: "x,y,width,height,opacity,rotation"
                    duration: 200
                }
            }
        ]
*/
    }

    Item { height: 4; width: 1 }
}

