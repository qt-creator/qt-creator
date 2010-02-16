import Qt 4.6

/*
    ListModel {
        id: libraryModel
        ListElement {
            sectionTitle: "Section 1"
            sectionEntries: [
                ListElement { itemLibId: 0; itemName: "Comp"; itemIconPath: "../images/default-icon.png" },
                ...
            ]
        }
        ...
    }
*/

/* workaround: ListView reports bogus viewportHeight

ListView {
    id: itemsView

    property string name: "itemsFlickable"
    anchors.fill: parent

    interactive: false

    model: itemsView.model
    delegate: sectionDelegate
}
*/


Rectangle {
    id: itemsView

    signal itemSelected(int itemLibId)
    signal itemDragged(int itemLibId)

    function expandAll() {
        expandAllEntries();
        scrollbar.handleBar.y = 0;
    }

    signal expandAllEntries()

    property int entriesPerRow: Math.max(1, Math.floor((itemsFlickable.width - 2) / style.cellWidth))
    property int cellWidth: Math.floor((itemsFlickable.width - 2) / entriesPerRow)
    property int cellHeight: style.cellHeight

    property var style
    style: ItemsViewStyle {}

    color: style.backgroundColor

    /* workaround: without this, a completed drag and drop operation would
                   result in the drag being continued when QmlView re-gains
                   focus */
    signal stopDragAndDrop
    MouseRegion {
        anchors.fill: parent
        hoverEnabled: true
        onEntered: if (!pressed) itemsView.stopDragAndDrop
    }

    Component {
        id: sectionDelegate

        SectionView {
            id: section
            style: itemsView.style

            entriesPerRow: itemsView.entriesPerRow
            cellWidth: itemsView.cellWidth
            cellHeight: itemsView.cellHeight

            width: itemsFlickable.width
            itemHighlight: selector

            onItemSelected: itemsView.itemSelected(itemLibId)
            onItemDragged: itemsView.itemDragged(itemLibId)

            function focusSelection() {
                itemSelection.focusSelection()
            }

            Connection {
                sender: itemsView
                signal: "expandAllEntries()"
                script: section.expand()
            }
        }
    }

    Flickable {
        id: itemsFlickable

        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: scrollbar.left
        anchors.rightMargin: 6
        clip: true

        interactive: false
        viewportHeight: col.height

        onViewportHeightChanged: scrollbar.limitHandle()

        Column {
            id: col

            Repeater {
                model: itemLibraryModel
                delegate: sectionDelegate
            }
        }

        Selector {
            id: selector
            z: -1
            style: itemsView.style
            scrollFlickable: itemsFlickable

            onMoveScrollbarHandle: scrollbar.moveHandle(viewportPos)

            width: itemsView.cellWidth
            height: itemsView.cellHeight
        }
    }

    Scrollbar {
        id: scrollbar

        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: parent.right
        anchors.leftMargin: -10
        anchors.right: parent.right

        scrollFlickable: itemsFlickable
        style: itemsView.style
    }
}

