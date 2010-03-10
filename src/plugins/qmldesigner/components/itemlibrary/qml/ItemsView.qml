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

/* The view displaying the item grid.

   The following Qml context properties have to be set:
     - listmodel itemLibraryModel
     - int itemLibraryIconWidth
     - int itemLibraryIconHeight

   itemLibraryModel has to have the following structure:

   ListModel {
       ListElement {
           int sectionLibId
           string sectionName
           list sectionEntries: [
               ListElement {
                   int itemLibId
                   string itemName
                   pixmap itemPixmap
               },
               ...
           ]
       }
       ...
   }
*/

Rectangle {
    id: itemsView

    // public

    function resetView() {
        expandAllEntries()
        scrollbar.reset()
    }

    signal itemSelected(int itemLibId)
    signal itemDragged(int itemLibId)

    signal stopDragAndDrop

    // internal

    signal expandAllEntries

    ItemsViewStyle { id: style }

    color: style.backgroundColor

    /* workaround: without this, a completed drag and drop operation would
                   result in the drag being continued when QmlView re-gains
                   focus */
    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        onEntered: {
	    if (!pressed)
		stopDragAndDrop()
	}
    }

    signal selectionUpdated(int itemSectionIndex)

    property int selectedItemLibId: -1
    property int selectionSectionLibId: -1

    function setSelection(itemLibId) {
	selectedItemLibId = itemLibId
	selectionSectionLibId = itemLibraryModel.getSectionLibId(itemLibId)
	selectionUpdated(itemLibraryModel.getItemSectionIndex(itemLibId))
    }

    function unsetSelection() {
	selectedItemLibId = -1
	selectionSectionLibId = -1
	selectionUpdated(-1)
    }

    Connections {
	target: itemLibraryModel
        onVisibilityChanged: {
	    if (itemLibraryModel.isItemVisible(selectedItemLibId))
		setSelection(selectedItemLibId)
	    else
		unsetSelection()
	}
    }

    /* the following 3 properties are calculated here for performance
       reasons and then passed to the section views */
    property int entriesPerRow: Math.max(1, Math.floor((itemsFlickable.width - 2) / style.cellWidth))
    property int cellWidth: Math.floor((itemsFlickable.width - 2) / entriesPerRow)
    property int cellHeight: style.cellHeight

    Component {
        id: sectionDelegate

        SectionView {
            id: section

            entriesPerRow: itemsView.entriesPerRow
            cellWidth: itemsView.cellWidth
            cellHeight: itemsView.cellHeight

            width: itemsFlickable.width
            itemHighlight: selector

	    property bool containsSelection: (selectionSectionLibId == sectionLibId)

            onItemSelected: {
		itemsView.setSelection(itemLibId)
		itemsView.itemSelected(itemLibId)
	    }
            onItemDragged: {
		section.itemSelected(itemLibId)
		itemsView.itemDragged(itemLibId)
	    }

            Connections {
                target: itemsView
                onExpandAllEntries: section.expand()
		onSelectionUpdated: {
		    if (containsSelection) {
			section.setSelection(itemSectionIndex)
			section.focusSelection(itemsFlickable)
		    } else
			section.unsetSelection()
		}
	    }

	    Component {
		id: selector

		Selector {
		    x: containsSelection? section.currentItem.x:0
		    y: containsSelection? section.currentItem.y:0
		    width: itemsView.cellWidth
		    height: itemsView.cellHeight

		    visible: containsSelection
		}
	    }
        }
    }

    Flickable {
        id: itemsFlickable

        anchors.top: parent.top
	anchors.topMargin: 3
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: scrollbar.left
	overShoot: false

        interactive: false
        contentHeight: col.height

	/* Limit the content position. Without this, resizing would get the
           content position out of scope regarding the scrollbar. */
	function limitContentPos() {
	    if (contentY < 0)
		contentY = 0;
	    else {
		var maxContentY = Math.max(0, contentHeight - height)
		if (contentY > maxContentY)
		    contentY = maxContentY;
	    }
	}
	onHeightChanged: limitContentPos()
	onContentHeightChanged: limitContentPos()

        Column {
            id: col

            Repeater {
                model: itemLibraryModel  // to be set in Qml context
                delegate: sectionDelegate
            }
        }
    }

    Scrollbar {
        id: scrollbar

        anchors.top: parent.top
        anchors.topMargin: 2
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 2
        anchors.left: parent.right
        anchors.leftMargin: -10
        anchors.right: parent.right

        flickable: itemsFlickable
    }
}
