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

