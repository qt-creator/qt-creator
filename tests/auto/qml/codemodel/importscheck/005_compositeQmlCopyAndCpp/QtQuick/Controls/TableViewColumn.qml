// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick 2.1

/*!
    \qmltype TableViewColumn
    \inqmlmodule QtQuick.Controls
    \since 5.1
    \ingroup viewitems
    \brief Used to define columns in a \l TableView.
*/

QtObject {

    /*! \internal */
    property Item __view: null

    /*! The title text of the column. */
    property string title

    /*! The model \c role of the column. */
    property string role

    /*! The current width of the column
    The default value depends on platform. If only one
    column is defined, the width expands to the viewport.
    */
    property int width: (__view && __view.columnCount === 1) ? __view.viewport.width : 160

    /*! The visible status of the column. */
    property bool visible: true

    /*! Determines if the column should be resizable.
    \since QtQuick.Controls 1.1 */
    property bool resizable: true

    /*! Determines if the column should be movable.
    The default value is \c true.
    \note A non-movable column may get indirectly moved if adjacent columns are movable.
    \since QtQuick.Controls 1.1 */
    property bool movable: true

    /*! \qmlproperty enumeration TableViewColumn::elideMode
    The text elide mode of the column.
    Allowed values are:
    \list
        \li Text.ElideNone
        \li Text.ElideLeft
        \li Text.ElideMiddle
        \li Text.ElideRight - the default
    \endlist
    \sa {QtQuick2::}{Text::elide} */
    property int elideMode: Text.ElideRight

    /*! \qmlproperty enumeration TableViewColumn::horizontalAlignment
    The horizontal text alignment of the column.
    Allowed values are:
    \list
        \li Text.AlignLeft - the default
        \li Text.AlignRight
        \li Text.AlignHCenter
        \li Text.AlignJustify
    \endlist
    \sa {QtQuick2::}{Text::horizontalAlignment} */
    property int horizontalAlignment: Text.AlignLeft

    /*! The delegate of the column. This can be used to set the
    \l TableView::itemDelegate for a specific column.

    In the delegate you have access to the following special properties:
    \list
    \li  styleData.selected - if the item is currently selected
    \li  styleData.value - the value or text for this item
    \li  styleData.textColor - the default text color for an item
    \li  styleData.row - the index of the row
    \li  styleData.column - the index of the column
    \li  styleData.elideMode - the elide mode of the column
    \li  styleData.textAlignment - the horizontal text alignment of the column
    \endlist
    */
    property Component delegate

    Accessible.role: Accessible.ColumnHeader
}
