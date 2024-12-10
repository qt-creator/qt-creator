/*
 Copyright (c) 2008-2024, Benoit AUTHEMAN All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the author or Destrat.io nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL AUTHOR BE LIABLE FOR ANY
 DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

//-----------------------------------------------------------------------------
// This file is a part of the QuickQanava software library.
//
// \file    qanTableGroupItem.h
// \author  benoit@destrat.io
// \date    2023 01 26
//-----------------------------------------------------------------------------

#pragma once

// Qt headers
#include <QQuickItem>

namespace qan { // ::qan

class Graph;
class TableGroup;
class NodeItem;

class TableCell : public QQuickItem
{
    /*! \name TableCell Object Management *///---------------------------------
    //@{
    Q_OBJECT
    QML_NAMED_ELEMENT(AbstractTableCell)
public:
    explicit TableCell(QQuickItem* parent = nullptr);
    virtual ~TableCell() override = default;
    TableCell(const TableCell&) = delete;
    //@}
    //-------------------------------------------------------------------------

    /*! \name Cell Container Management *///-----------------------------------
    //@{
public:
    //! \copydoc _table
    Q_PROPERTY(qan::TableGroup* table READ getTable NOTIFY tableChanged)
    //! \copydoc _table
    const qan::TableGroup*  getTable() const;
    //! \copydoc _table
    qan::TableGroup*        getTable();
    //! \copydoc _table
    void                    setTable(qan::TableGroup* table);
protected:
    //! Cell parent table.
    QPointer<qan::TableGroup>   _table;
signals:
    //! \copydoc _table
    void                    tableChanged();

public:
    //! \copydoc setItem()
    Q_PROPERTY(QQuickItem* item READ getItem NOTIFY itemChanged)
    //! \copydoc setItem()
    const QQuickItem*       getItem() const { return _item.data(); }
    //! \copydoc setItem()
    QQuickItem*             getItem() { return _item.data(); }
    //! Set `item` in this cell "container", `item` is reparented to cell.
    void                    setItem(QQuickItem* item);
protected:
    //! \copydoc getItem()
    QPointer<QQuickItem>    _item;
signals:
    //! \copydoc setItem()
    void                    itemChanged();

public:
    //! Restore the cache initialized in `setItem()`.
    void    restoreCache(qan::NodeItem* nodeItem) const;
private:
    // Private cache used to restore `item` once it is dragged out of the cell.
    bool    _cacheSelectable = true;
    bool    _cacheDraggable = true;
    bool    _cacheResizable = true;
    QSizeF  _cacheSize = QSizeF{0., 0.};

protected slots:
    //! Fit actual `_item` to this cell.
    void                    fitItemToCell();

public:
    //! \copydoc setUserProp()
    const QVariant&     getUserProp() const { return _userProp; }
    //! User defined property (may be usefull for custom serialization/persistence, or storing UUID).
    void                setUserProp(const QVariant& userProp) { _userProp = userProp; }
protected:
    //! \copydoc getUserProp()
    QVariant            _userProp;
    //@}
    //-------------------------------------------------------------------------
};

} // ::qan
