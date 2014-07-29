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
