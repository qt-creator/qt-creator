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
import QtQuick.Controls.Private 1.0
import QtQuick.Controls.Styles 1.1

/*!
   \qmltype TableView
   \inqmlmodule QtQuick.Controls
   \since 5.1
   \ingroup views
   \brief Provides a list view with scroll bars, styling and header sections.

   \image tableview.png

   A TableView is similar to \l ListView, and adds scroll bars, selection, and
   resizable header sections. As with \l ListView, data for each row is provided through a \l model:

 \code
 ListModel {
    id: libraryModel
    ListElement{ title: "A Masterpiece" ; author: "Gabriel" }
    ListElement{ title: "Brilliance"    ; author: "Jens" }
    ListElement{ title: "Outstanding"   ; author: "Frederik" }
 }
 \endcode

   You provide title and size of a column header
   by adding a \l TableViewColumn as demonstrated below.
 \code

 TableView {
    TableViewColumn{ role: "title"  ; title: "Title" ; width: 100 }
    TableViewColumn{ role: "author" ; title: "Author" ; width: 200 }
    model: libraryModel
 }
 \endcode

   The header sections are attached to values in the \l model by defining
   the model role they attach to. Each property in the model will
   then be shown in their corresponding column.

   You can customize the look by overriding the \l itemDelegate,
   \l rowDelegate, or \l headerDelegate properties.

   The view itself does not provide sorting. This has to
   be done on the model itself. However you can provide sorting
   on the model, and enable sort indicators on headers.

\list
    \li int sortIndicatorColumn - The index of the current sort column
    \li bool sortIndicatorVisible - Whether the sort indicator should be enabled
    \li enum sortIndicatorOrder - Qt.AscendingOrder or Qt.DescendingOrder depending on state
\endlist

    You can create a custom appearance for a TableView by
    assigning a \l {QtQuick.Controls.Styles::TableViewStyle}{TableViewStyle}.
*/

ScrollView {
    id: root

    /*! \qmlproperty model TableView::model
    This property holds the model providing data for the table view.

    The model provides the set of data that is used to create the items in the view.
    Models can be created directly in QML using ListModel, XmlListModel or VisualItemModel,
    or provided by C++ model classes. \sa ListView::model

    Example model:

    \code
    model: ListModel {
        ListElement{ column1: "value 1" ; column2: "value 2" }
        ListElement{ column1: "value 3" ; column2: "value 4" }
    }
    \endcode
    \sa {qml-data-models}{Data Models}
    */
    property var model

    /*! This property is set to \c true if the view alternates the row color.
        The default value is \c true. */
    property bool alternatingRowColors: true

    /*! This property determines if the header is visible.
        The default value is \c true. */
    property bool headerVisible: true

    /*! \qmlproperty bool TableView::backgroundVisible

        This property determines if the background should be filled or not.

        The default value is \c true.

        \note The rowDelegate is not affected by this property
    */
    property alias backgroundVisible: colorRect.visible

    /*! This property defines a delegate to draw a specific cell.

    In the item delegate you have access to the following special properties:
    \list
    \li  styleData.selected - if the item is currently selected
    \li  styleData.value - the value or text for this item
    \li  styleData.textColor - the default text color for an item
    \li  styleData.row - the index of the row
    \li  styleData.column - the index of the column
    \li  styleData.elideMode - the elide mode of the column
    \li  styleData.textAlignment - the horizontal text alignment of the column
    \endlist

    Example:
    \code
    itemDelegate: Item {
        Text {
            anchors.verticalCenter: parent.verticalCenter
            color: styleData.textColor
            elide: styleData.elideMode
            text: styleData.value
        }
    }
    \endcode */
    property Component itemDelegate: __style ? __style.itemDelegate : null

    /*! This property defines a delegate to draw a row.

    In the row delegate you have access to the following special properties:
    \list
    \li  styleData.alternate - true when the row uses the alternate background color
    \li  styleData.selected - true when the row is currently selected
    \li  styleData.row - the index of the row
    \endlist
    */
    property Component rowDelegate: __style ? __style.rowDelegate : null

    /*! This property defines a delegate to draw a header.

    In the header delegate you have access to the following special properties:
    \list
    \li  styleData.value - the value or text for this item
    \li  styleData.column - the index of the column
    \li  styleData.pressed - true when the column is being pressed
    \li  styleData.containsMouse - true when the column is under the mouse
    \li  styleData.textAlignment - the horizontal text alignment of the column (since QtQuickControls 1.1)
    \endlist
    */
    property Component headerDelegate: __style ? __style.headerDelegate : null

    /*! Index of the current sort column.
        The default value is \c {0}. */
    property int sortIndicatorColumn

    /*! This property shows or hides the sort indicator
        The default value is \c false.
        \note The view itself does not sort the data. */
    property bool sortIndicatorVisible: false

    /*!
       \qmlproperty enumeration TableView::sortIndicatorOrder

       This sets the sorting order of the sort indicator
       The allowed values are:
       \list
       \li Qt.AscendingOrder - the default
       \li Qt.DescendingOrder
       \endlist
    */
    property int sortIndicatorOrder: Qt.AscendingOrder

    /*! \internal */
    default property alias __columns: root.data

    /*! \qmlproperty Component TableView::contentHeader
    This is the content header of the TableView */
    property alias contentHeader: listView.header

    /*! \qmlproperty Component TableView::contentFooter
    This is the content footer of the TableView */
    property alias contentFooter: listView.footer

    /*! \qmlproperty int TableView::rowCount
    The current number of rows */
    readonly property alias rowCount: listView.count

    /*! \qmlproperty int TableView::columnCount
    The current number of columns */
    readonly property alias columnCount: columnModel.count

    /*! \qmlproperty string TableView::section.property
        \qmlproperty enumeration TableView::section.criteria
        \qmlproperty Component TableView::section.delegate
        \qmlproperty enumeration TableView::section.labelPositioning
    These properties determine the section labels.
    \sa ListView::section */
    property alias section: listView.section

    /*! \qmlproperty int TableView::currentRow
    The current row index of the view.
    The default value is \c -1 to indicate that no row is selected.
    */
    property alias currentRow: listView.currentIndex

    /*! \internal */
    property alias __currentRowItem: listView.currentItem

    /*! \qmlsignal TableView::activated(int row)

        Emitted when the user activates an item by mouse or keyboard interaction.
        Mouse activation is triggered by single- or double-clicking, depending on the platform.

        \a row int provides access to the activated row index.

        \note This signal is only emitted for mouse interaction that is not blocked in the row or item delegate.
    */
    signal activated(int row)

    /*! \qmlsignal TableView::clicked(int row)

        Emitted when the user clicks a valid row by single clicking

        \a row int provides access to the clicked row index.

        \note This signal is only emitted if the row or item delegate does not accept mouse events.
    */
    signal clicked(int row)

    /*! \qmlsignal TableView::doubleClicked(int row)

        Emitted when the user double clicks a valid row.

        \a row int provides access to the clicked row index.

        \note This signal is only emitted if the row or item delegate does not accept mouse events.
    */
    signal doubleClicked(int row)

    /*!
        \qmlmethod TableView::positionViewAtRow( int row, PositionMode mode )

    Positions the view such that the specified \a row is at the position defined by \a mode:
       \list
       \li ListView.Beginning - position item at the top of the view.
       \li ListView.Center - position item in the center of the view.
       \li ListView.End - position item at bottom of the view.
       \li ListView.Visible - if any part of the item is visible then take no action, otherwise bring the item into view.
       \li ListView.Contain - ensure the entire item is visible. If the item is larger than the view the item is positioned
           at the top of the view.
       \endlist

    If positioning the \a row creates an empty space at the beginning
    or end of the view, then the view is positioned at the boundary.

    For example, to position the view at the end at startup:

    \code
    Component.onCompleted: table.positionViewAtRow(rowCount -1, ListView.Contain)
    \endcode

    Depending on how the model is populated, the model may not be ready when
    TableView Component.onCompleted is called. In that case you may need to
    delay the call to positionViewAtRow by using a \l {QtQml::Timer}{Timer}.

    \note This method should only be called after the component has completed.
    */

    function positionViewAtRow(row, mode) {
        listView.positionViewAtIndex(row, mode)
    }

    /*!
        \qmlmethod int TableView::rowAt( int x, int y )

        Returns the index of the visible row at the point \a x, \a y in content
        coordinates. If there is no visible row at the point specified, \c -1 is returned.

        \note This method should only be called after the component has completed.
    */

    function rowAt(x, y) {
        var obj = root.mapToItem(listView.contentItem, x, y)
        return listView.indexAt(obj.x, obj.y)
    }

    /*! Adds a \a column and returns the added column.

        The \a column argument can be an instance of TableViewColumn,
        or a Component. The component has to contain a TableViewColumn.
        Otherwise  \c null is returned.
    */
    function addColumn(column) {
        return insertColumn(columnCount, column)
    }

    /*! Inserts a \a column at the given \a index and returns the inserted column.

        The \a column argument can be an instance of TableViewColumn,
        or a Component. The component has to contain a TableViewColumn.
        Otherwise  \c null is returned.
    */
    function insertColumn(index, column) {
        var object = column
        if (typeof column['createObject'] === 'function')
            object = column.createObject(root)

        else if (object.__view) {
            console.warn("TableView::insertColumn(): you cannot add a column to multiple views")
            return null
        }
        if (index >= 0 && index <= columnCount && object.Accessible.role === Accessible.ColumnHeader) {
            object.__view = root
            columnModel.insert(index, {columnItem: object})
            return object
        }

        if (object !== column)
            object.destroy()
        console.warn("TableView::insertColumn(): invalid argument")
        return null
    }

    /*! Removes and destroys a column at the given \a index. */
    function removeColumn(index) {
        if (index < 0 || index >= columnCount) {
            console.warn("TableView::removeColumn(): invalid argument")
            return
        }
        var column = columnModel.get(index).columnItem
        columnModel.remove(index, 1)
        column.destroy()
    }

    /*! Moves a column \a from index \a to another. */
    function moveColumn(from, to) {
        if (from < 0 || from >= columnCount || to < 0 || to >= columnCount) {
            console.warn("TableView::moveColumn(): invalid argument")
            return
        }
        columnModel.move(from, to, 1)
    }

    /*! Returns the column at the given \a index
        or \c null if the \a index is invalid. */
    function getColumn(index) {
        if (index < 0 || index >= columnCount)
            return null
        return columnModel.get(index).columnItem
    }

    /*! \qmlproperty Selection TableView::selection
    \since QtQuick.Controls 1.1

    This property contains the current row-selection of the \l TableView.
    The selection allows you to select, deselect or iterate over selected rows.

    \list
    \li function \b clear() - deselects all rows
    \li function \b selectAll() - selects all rows
    \li function \b select(from, to) - select a range
    \li functton \b deselect(from, to) - de-selects a range
    \li function \b forEach(callback) - Allows you to iterate over selected rows
    \li function \b contains(index) - Allows you to iterate over selected rows
    \li signal \b selectionChanged() - The current row selection changed
    \li readonly property int \b count - The number of selected rows
    \endlist

    \b Example:
    \code
        tableview.selection.select(0)       // select row index 0

        tableview.selection.select(1, 3)    // select row indexes 1, 2 and 3

        tableview.selection.deselect(0, 1)  // deselects row index 0 and 1

        tableview.selection.deselect(2)     // deselects row index 2
    \endcode

    \b Example: To iterate over selected indexes, you can pass a callback function.
                \a rowIndex is passed as as an argument to the callback function.
    \code
        tableview.selection.forEach( function(rowIndex) {console.log(rowIndex)} )
    \endcode

    */

    readonly property alias selection: selectionObject

    /*!
        \qmlproperty enumeration TableView::selectionMode
        \since QtQuick.Controls 1.1

        This enum indicates how the view responds to user selections:

        The possible modes are:

        \list

        \li SelectionMode.NoSelection - Items cannot be selected.

        \li SelectionMode.SingleSelection - When the user selects an item,
            any already-selected item becomes unselected, and the user cannot
            unselect the selected item. (Default)

        \li SelectionMode.MultiSelection - When the user selects an item in the usual way,
            the selection status of that item is toggled and the other items are left alone.

        \li SelectionMode.ExtendedSelection - When the user selects an item in the usual way,
            the selection is cleared and the new item selected. However, if the user presses the
            Ctrl key when clicking on an item, the clicked item gets toggled and all other items
            are left untouched. If the user presses the Shift key while clicking
            on an item, all items between the current item and the clicked item are selected or unselected,
            depending on the state of the clicked item. Multiple items can be selected by dragging the
            mouse over them.

        \li SelectionMode.ContiguousSelection - When the user selects an item in the usual way,
            the selection is cleared and the new item selected. However, if the user presses the Shift key while
            clicking on an item, all items between the current item and the clicked item are selected.

        \endlist
    */
    property int selectionMode: SelectionMode.SingleSelection

    Component.onCompleted: {
        for (var i = 0; i < __columns.length; ++i) {
            var column = __columns[i]
            if (column.Accessible.role === Accessible.ColumnHeader)
                addColumn(column)
        }
    }

    style: Qt.createComponent(Settings.style + "/TableViewStyle.qml", root)


    Accessible.role: Accessible.Table

    implicitWidth: 200
    implicitHeight: 150

    frameVisible: true
    __scrollBarTopMargin: (__style && __style.transientScrollBars || Qt.platform.os === "osx") ? headerrow.height : 0
    __viewTopMargin: headerrow.height

    /*! \internal */
    property bool __activateItemOnSingleClick: __style ? __style.activateItemOnSingleClick : false

    /*! \internal */
    function __decrementCurrentIndex() {
        __scroller.blockUpdates = true;
        listView.decrementCurrentIndex();
        __scroller.blockUpdates = false;

        var newIndex = listView.indexAt(0, listView.contentY)
        if (newIndex !== -1) {
            if (selectionMode > SelectionMode.SingleSelection)
                mousearea.dragRow = newIndex
            else if (selectionMode === SelectionMode.SingleSelection)
                selection.__selectOne(newIndex)
        }
    }

    /*! \internal */
    function __incrementCurrentIndex() {
        __scroller.blockUpdates = true;
        listView.incrementCurrentIndex();
        __scroller.blockUpdates = false;

        var newIndex = Math.max(0, listView.indexAt(0, listView.height + listView.contentY))
        if (newIndex !== -1) {
            if (selectionMode > SelectionMode.SingleSelection)
                mousearea.dragRow = newIndex
            else if (selectionMode === SelectionMode.SingleSelection)
                selection.__selectOne(newIndex)
        }
    }

    onModelChanged: selection.clear()

    ListView {
        id: listView
        focus: true
        activeFocusOnTab: true
        anchors.topMargin: tableHeader.height
        anchors.fill: parent
        currentIndex: -1
        visible: columnCount > 0
        interactive: Settings.hasTouchScreen

        SystemPalette {
            id: palette
            colorGroup: enabled ? SystemPalette.Active : SystemPalette.Disabled
        }

        Rectangle {
            id: colorRect
            parent: viewport
            anchors.fill: parent
            color: __style ? __style.backgroundColor : palette.base
            z: -2
        }

        MouseArea {
            id: mousearea

            z: -1
            anchors.fill: listView
            propagateComposedEvents: true

            property bool autoincrement: false
            property bool autodecrement: false
            property int mouseModifiers: 0
            property int previousRow: 0
            property int clickedRow: -1
            property int dragRow: -1
            property int firstKeyRow: -1

            onReleased: {
                autoincrement = false
                autodecrement = false
                var clickIndex = listView.indexAt(0, mouseY + listView.contentY)
                if (clickIndex > -1) {
                    if (Settings.hasTouchScreen) {
                        listView.currentIndex = clickIndex
                        mouseSelect(clickIndex, mouse.modifiers)
                    }
                    previousRow = clickIndex
                }

                if (mousearea.dragRow >= 0) {
                    selection.__select(selection.contains(mousearea.clickedRow), mousearea.clickedRow, mousearea.dragRow)
                    mousearea.dragRow = -1
                }
            }

            // Handle vertical scrolling whem dragging mouse outside boundraries
            Timer { running: mousearea.autoincrement && __verticalScrollBar.visible; repeat: true; interval: 20 ; onTriggered: __incrementCurrentIndex()}
            Timer { running: mousearea.autodecrement && __verticalScrollBar.visible; repeat: true; interval: 20 ; onTriggered: __decrementCurrentIndex()}

            onPositionChanged: {
                if (mouseY > listView.height && pressed) {
                    if (autoincrement) return;
                    autodecrement = false;
                    autoincrement = true;
                } else if (mouseY < 0 && pressed) {
                    if (autodecrement) return;
                    autoincrement = false;
                    autodecrement = true;
                } else  {
                    autoincrement = false;
                    autodecrement = false;
                }

                if (pressed && !Settings.hasTouchScreen) {
                    var newIndex = Math.max(0, listView.indexAt(0, mouseY + listView.contentY))
                    if (newIndex >= 0 && newIndex != currentRow) {
                        listView.currentIndex = newIndex;
                        if (selectionMode === SelectionMode.SingleSelection) {
                            selection.__selectOne(newIndex)
                        } else if (selectionMode > 1) {
                            dragRow = newIndex
                        }
                    }
                }
                mouseModifiers = mouse.modifiers
            }

            onClicked: {
                var clickIndex = listView.indexAt(0, mouseY + listView.contentY)
                if (clickIndex > -1) {
                    if (root.__activateItemOnSingleClick)
                        root.activated(clickIndex)
                    root.clicked(clickIndex)
                }
            }

            onPressed: {
                var newIndex = listView.indexAt(0, mouseY + listView.contentY)
                listView.forceActiveFocus()
                if (newIndex > -1 && !Settings.hasTouchScreen) {
                    listView.currentIndex = newIndex
                    mouseSelect(newIndex, mouse.modifiers)
                    mousearea.clickedRow = newIndex
                }
                mouseModifiers = mouse.modifiers
            }

            function mouseSelect(index, modifiers) {
                if (selectionMode) {
                    if (modifiers & Qt.ShiftModifier && (selectionMode === SelectionMode.ExtendedSelection)) {
                        selection.select(previousRow, index)
                    } else if (selectionMode === SelectionMode.MultiSelection ||
                               (selectionMode === SelectionMode.ExtendedSelection && modifiers & Qt.ControlModifier)) {
                        selection.__select(!selection.contains(index) , index)
                    } else {
                        selection.__selectOne(index)
                    }
                }
            }

            onDoubleClicked: {
                var clickIndex = listView.indexAt(0, mouseY + listView.contentY)
                if (clickIndex > -1) {
                    if (!root.__activateItemOnSingleClick)
                        root.activated(clickIndex)
                    root.doubleClicked(clickIndex)
                }
            }

            // Note:  with boolean preventStealing we are keeping the flickable from
            // eating our mouse press events
            preventStealing: !Settings.hasTouchScreen

            TableViewSelection { id: selectionObject }
        }

        // Fills extra rows with alternate color
        Column {
            id: rowfiller
            Loader {
                id: rowSizeItem
                sourceComponent: root.rowDelegate
                visible: false
                property QtObject styleData: QtObject {
                    property bool alternate: false
                    property bool selected: false
                    property bool hasActiveFocus: false
                }
            }
            property int rowHeight: rowSizeItem.implicitHeight
            property int paddedRowCount: height/rowHeight
            property int count: listView.count
            y: listView.contentHeight
            width: parent.width
            visible: alternatingRowColors
            height: viewport.height - listView.contentHeight
            Repeater {
                model: visible ? parent.paddedRowCount : 0
                Loader {
                    width: rowfiller.width
                    height: rowfiller.rowHeight
                    sourceComponent: root.rowDelegate
                    property QtObject styleData: QtObject {
                        readonly property bool alternate: (index + rowCount) % 2 === 1
                        readonly property bool selected: false
                        readonly property bool hasActiveFocus: root.activeFocus
                    }
                    readonly property var model: listView.model
                    readonly property var modelData: null
                }
            }
        }

        ListModel {
            id: columnModel
        }

        highlightFollowsCurrentItem: true
        model: root.model

        function keySelect(shiftPressed, row) {
            if (row < 0 || row === rowCount - 1)
                return
            if (shiftPressed && (selectionMode >= SelectionMode.ExtendedSelection)) {
                selection.__ranges = new Array()
                selection.select(mousearea.firstKeyRow, row)
            } else {
                selection.__selectOne(row)
            }
        }

        Keys.onUpPressed: {
            event.accepted = false
            __scroller.blockUpdates = true;
            listView.decrementCurrentIndex();
            __scroller.blockUpdates = false;
            if (selectionMode)
                keySelect(event.modifiers & Qt.ShiftModifier, currentRow)
        }

        Keys.onDownPressed: {
            event.accepted = false
            __scroller.blockUpdates = true;
            listView.incrementCurrentIndex();
            __scroller.blockUpdates = false;
            if (selectionMode)
                keySelect(event.modifiers & Qt.ShiftModifier, currentRow)
        }

        Keys.onPressed: {
            if (event.key === Qt.Key_PageUp) {
                __verticalScrollBar.value = __verticalScrollBar.value - listView.height
            } else if (event.key === Qt.Key_PageDown)
                __verticalScrollBar.value = __verticalScrollBar.value + listView.height

            if (event.key === Qt.Key_Shift) {
                mousearea.firstKeyRow = currentRow
            }

            if (event.key === Qt.Key_A && event.modifiers & Qt.ControlModifier) {
                if (selectionMode > 1)
                    selection.selectAll()
            }
        }

        Keys.onReleased: {
            if (event.key === Qt.Key_Shift)
                mousearea.firstKeyRow = -1
        }

        Keys.onReturnPressed: {
            event.accepted = false
            if (currentRow > -1)
                root.activated(currentRow);
        }

        delegate: FocusScope {
            id: rowitem
            width: itemrow.width
            height: rowstyle.height

            function selected() {
                if (mousearea.dragRow > -1 && (rowIndex >= mousearea.clickedRow && rowIndex <= mousearea.dragRow
                        || rowIndex <= mousearea.clickedRow && rowIndex >=mousearea.dragRow))
                    return selection.contains(mousearea.clickedRow)

                return selection.count && selection.contains(rowIndex)
            }
            readonly property int rowIndex: model.index
            readonly property bool alternate: alternatingRowColors && rowIndex % 2 == 1
            readonly property var itemModelData: typeof modelData == "undefined" ? null : modelData
            readonly property var itemModel: model
            readonly property bool itemSelected: selected()
            readonly property color itemTextColor: itemSelected ? __style.highlightedTextColor : __style.textColor

            onActiveFocusChanged: {
                if (activeFocus)
                    listView.currentIndex = rowIndex
            }

            Loader {
                id: rowstyle
                // row delegate
                sourceComponent: root.rowDelegate
                // Row fills the view width regardless of item size
                // But scrollbar should not adjust to it
                height: item ? item.height : 16
                width: parent.width + __horizontalScrollBar.width
                x: listView.contentX

                // these properties are exposed to the row delegate
                // Note: these properties should be mirrored in the row filler as well
                property QtObject styleData: QtObject {
                    readonly property int row: rowitem.rowIndex
                    readonly property bool alternate: rowitem.alternate
                    readonly property bool selected: rowitem.itemSelected
                    readonly property bool hasActiveFocus: root.activeFocus
                }
                readonly property var model: listView.model
                readonly property var modelData: rowitem.itemModelData
            }
            Row {
                id: itemrow
                height: parent.height
                Repeater {
                    id: repeater
                    model: columnModel

                    Loader {
                        id: itemDelegateLoader
                        width:  columnItem.width
                        height: parent ? parent.height : 0
                        visible: columnItem.visible
                        sourceComponent: columnItem.delegate ? columnItem.delegate : itemDelegate

                        // these properties are exposed to the item delegate
                        readonly property var model: listView.model
                        readonly property var modelData: itemModelData

                        property QtObject styleData: QtObject {
                            readonly property int row: rowitem.rowIndex
                            readonly property int column: index
                            readonly property int elideMode: columnItem.elideMode
                            readonly property int textAlignment: columnItem.horizontalAlignment
                            readonly property bool selected: rowitem.itemSelected
                            readonly property color textColor: rowitem.itemTextColor
                            readonly property string role: columnItem.role
                            readonly property var value: itemModel.hasOwnProperty(role)
                                                         ? itemModel[role] // Qml ListModel and QAbstractItemModel
                                                         : modelData && modelData.hasOwnProperty(role)
                                                           ? modelData[role] // QObjectList / QObject
                                                           : modelData != undefined ? modelData : "" // Models without role
                        }
                    }
                }
                onWidthChanged: listView.contentWidth = width
            }
        }

        Text{ id:text }

        Item {
            id: tableHeader
            clip: true
            parent: __scroller
            visible: headerVisible
            anchors.top: parent.top
            anchors.topMargin: viewport.anchors.topMargin
            anchors.leftMargin: viewport.anchors.leftMargin
            anchors.margins: viewport.anchors.margins
            anchors.rightMargin: (frameVisible ? __scroller.rightMargin : 0) +
                                 (__scroller.outerFrame && __scrollBarTopMargin ? 0 : __verticalScrollBar.width
                                                          + __scroller.scrollBarSpacing + root.__style.padding.right)

            anchors.left: parent.left
            anchors.right: parent.right

            height: headerrow.height

            Row {
                id: headerrow
                x: -listView.contentX

                Repeater {
                    id: repeater

                    property int targetIndex: -1
                    property int dragIndex: -1

                    model: columnModel

                    delegate: Item {
                        z:-index
                        width: columnCount == 1 ? viewport.width + __verticalScrollBar.width : modelData.width
                        visible: modelData.visible
                        height: headerVisible ? headerStyle.height : 0

                        Loader {
                            id: headerStyle
                            sourceComponent: root.headerDelegate
                            anchors.left: parent.left
                            anchors.right: parent.right
                            property QtObject styleData: QtObject {
                                readonly property string value: modelData.title
                                readonly property bool pressed: headerClickArea.pressed
                                readonly property bool containsMouse: headerClickArea.containsMouse
                                readonly property int column: index
                                readonly property int textAlignment: modelData.horizontalAlignment
                            }
                        }
                        Rectangle{
                            id: targetmark
                            width: parent.width
                            height:parent.height
                            opacity: (index == repeater.targetIndex && repeater.targetIndex != repeater.dragIndex) ? 0.5 : 0
                            Behavior on opacity { NumberAnimation{duration:160}}
                            color: palette.highlight
                            visible: modelData.movable
                        }

                        MouseArea{
                            id: headerClickArea
                            drag.axis: Qt.YAxis
                            hoverEnabled: true
                            anchors.fill: parent
                            onClicked: {
                                if (sortIndicatorColumn == index)
                                    sortIndicatorOrder = sortIndicatorOrder == Qt.AscendingOrder ? Qt.DescendingOrder : Qt.AscendingOrder
                                sortIndicatorColumn = index
                            }
                            // Here we handle moving header sections
                            // NOTE: the direction is different from the master branch
                            // so this indicates that I am using an invalid assumption on item ordering
                            onPositionChanged: {
                                if (modelData.movable && pressed && columnCount > 1) { // only do this while dragging
                                    for (var h = columnCount-1 ; h >= 0 ; --h) {
                                        if (drag.target.x > headerrow.children[h].x) {
                                            repeater.targetIndex = h
                                            break
                                        }
                                    }
                                }
                            }

                            onPressed: {
                                repeater.dragIndex = index
                                draghandle.x = parent.x
                            }

                            onReleased: {
                                if (repeater.targetIndex >= 0 && repeater.targetIndex != index ) {
                                    var targetColumn = columnModel.get(repeater.targetIndex).columnItem
                                    if (targetColumn.movable) {
                                        columnModel.move(index, repeater.targetIndex, 1)
                                        if (sortIndicatorColumn == index)
                                            sortIndicatorColumn = repeater.targetIndex
                                    }
                                }
                                repeater.targetIndex = -1
                            }
                            drag.maximumX: 1000
                            drag.minimumX: -1000
                            drag.target: modelData.movable && columnCount > 1 ? draghandle : null
                        }

                        Loader {
                            id: draghandle
                            property QtObject styleData: QtObject{
                                readonly property string value: modelData.title
                                readonly property bool pressed: headerClickArea.pressed
                                readonly property bool containsMouse: headerClickArea.containsMouse
                                readonly property int column: index
                                readonly property int textAlignment: modelData.horizontalAlignment
                            }

                            parent: tableHeader
                            width: modelData.width
                            height: parent.height
                            sourceComponent: root.headerDelegate
                            visible: headerClickArea.pressed
                            opacity: 0.5
                        }


                        MouseArea {
                            id: headerResizeHandle
                            property int offset: 0
                            property int minimumSize: 20
                            anchors.rightMargin: -width/2
                            width: 16 ; height: parent.height
                            anchors.right: parent.right
                            enabled: modelData.resizable && columnCount > 1
                            onPositionChanged:  {
                                var newHeaderWidth = modelData.width + (mouseX - offset)
                                modelData.width = Math.max(minimumSize, newHeaderWidth)
                            }
                            property bool found:false

                            onDoubleClicked: {
                                var row
                                var minWidth =  0
                                var listdata = listView.children[0]
                                for (row = 0 ; row < listdata.children.length ; ++row){
                                    var item = listdata.children[row+1]
                                    if (item && item.children[1] && item.children[1].children[index] &&
                                            item.children[1].children[index].children[0].hasOwnProperty("implicitWidth"))
                                        minWidth = Math.max(minWidth, item.children[1].children[index].children[0].implicitWidth)
                                }
                                if (minWidth)
                                    modelData.width = minWidth
                            }
                            onPressedChanged: if (pressed) offset=mouseX
                            cursorShape: enabled ? Qt.SplitHCursor : Qt.ArrowCursor
                        }
                    }
                }
            }
            Loader {
                id: loader
                property QtObject styleData: QtObject{
                    readonly property string value: ""
                    readonly property bool pressed: false
                    readonly property bool containsMouse: false
                    readonly property int column: -1
                    readonly property int textAlignment: Text.AlignLeft
                }

                anchors.top: parent.top
                anchors.right: parent.right
                anchors.bottom: headerrow.bottom
                anchors.rightMargin: -2
                sourceComponent: root.headerDelegate
                width: root.width - headerrow.width + 2
                visible: root.columnCount
                z:-1
            }
        }
    }
}
