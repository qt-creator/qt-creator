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
// \file	qanTableCell.cpp
// \author	benoit@destrat.io
// \date	2023 01 26
//-----------------------------------------------------------------------------

// QuickQanava headers
#include "./qanTableCell.h"
#include "./qanNodeItem.h"
#include "./qanTableGroup.h"

namespace qan { // ::qan

/* TableCell Object Management *///--------------------------------------------
TableCell::TableCell(QQuickItem* parent):
    QQuickItem{parent}
{
    // Drops are managed by WuickQanava at qan::TableGroupItem level
    setFlag(QQuickItem::ItemAcceptsDrops, false);

    // Fit item in cell to cell width/height
    connect(this, &QQuickItem::widthChanged,
            this, &qan::TableCell::fitItemToCell);
    connect(this, &QQuickItem::heightChanged,
            this, &qan::TableCell::fitItemToCell);

    setClip(true);  // Clip content
}
//-----------------------------------------------------------------------------


/* Cell Container Management *///----------------------------------------------
const qan::TableGroup*  TableCell::getTable() const { return _table.data(); }
qan::TableGroup*        TableCell::getTable() { return _table.data(); }

void    TableCell::setTable(qan::TableGroup* table)
{
    if (table != _table) {
        _table = table;
        if (table != nullptr)
            connect(table,  &qan::TableGroup::cellTopPaddingChanged,    // Do not disconnect table is not subject to
                    this,   &qan::TableCell::fitItemToCell);            // change
        emit tableChanged();
    }
}

void    TableCell::setItem(QQuickItem* item)
{
    if (item != _item) {
        _item = item;
        if (_item) {
            _item->setX(0.);
            _item->setY(0.);
            _item->setParentItem(this);
            auto nodeItem = static_cast<qan::NodeItem*>(item);
            if (nodeItem != nullptr) {
                _cacheSelectable = nodeItem->getSelectable();  // Save node configuration in
                _cacheDraggable = nodeItem->getDraggable();    // local cache.
                _cacheResizable = nodeItem->getResizable();
                _cacheSize = nodeItem->size();

                nodeItem->setSelectable(false);
                nodeItem->setDraggable(false);
                nodeItem->setResizable(false);
            }
        }
        fitItemToCell();
        emit itemChanged();
    }
}

void    TableCell::restoreCache(qan::NodeItem* nodeItem) const
{
    if (nodeItem != nullptr) {
        nodeItem->setSelectable(_cacheSelectable);
        nodeItem->setDraggable(_cacheDraggable);
        nodeItem->setResizable(_cacheResizable);
        nodeItem->setSize(_cacheSize);
    }
}

void    TableCell::fitItemToCell()
{
    if (_item) {
        const auto topPadding = _table ? _table->getCellTopPadding() :
                                         0.;
        _item->setY(topPadding);
        _item->setWidth(width());
        _item->setHeight(height() - topPadding);
    }
}
//-----------------------------------------------------------------------------

} // ::qan
