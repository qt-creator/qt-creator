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
#include <QPointF>
#include <QPolygonF>

// QuickQanava headers
#include "./qanNodeItem.h"
#include "./qanGroupItem.h"
#include "./qanTableBorder.h"

namespace qan { // ::qan

class Graph;
class TableGroup;
class TableCell;

/*! \brief Visual delegate for qan::TableGroup, manage table layout, borders and cells.
 *
 * \nosubgrouping
 */
class TableGroupItem : public qan::GroupItem
{
    /*! \name TableGroupItem Object Management *///----------------------------
    //@{
    Q_OBJECT
    QML_ELEMENT
public:
    explicit TableGroupItem(QQuickItem* parent = nullptr);
    virtual ~TableGroupItem() override;
    TableGroupItem(const TableGroupItem&) = delete;

public:
    //! Override qan::GroupItem::setContainer()
    virtual bool    setContainer(QQuickItem* container) noexcept override;

signals:
    //! Emitted when the table geometry has change (border modified for example).
    void            modified();
    //@}
    //-------------------------------------------------------------------------


    /*! \name Borders and Cells Management *///--------------------------------
    //@{
public:
    //! Clear table borders and cells (cells nodes are reparented to group).
    void        clearLayout();

    //! Initialize a table withe default cells and borders.
    void        initialize(int cols, int rows);

    void        createCells(int cellsCount);
    void        createBorders(int verticalBordersCount, int horizontalBordersCount);

protected:
    auto        createFromComponent(QQmlComponent& component) -> QQuickItem*;

public:
    void        initializeTableLayout();
public slots:
    //! Layout current cell after a table geometry change.
    void        layoutTable();

    //! Layout table cells, triggered when table style change.
    void        layoutCells();

public:
    virtual bool            setGroup(qan::Group* group) noexcept override;
protected:
    const qan::TableGroup*  getTableGroup() const;
    qan::TableGroup*        getTableGroup();

public:
    using Cells_t = std::vector<qan::TableCell*>;
    using Borders_t = std::vector<qan::TableBorder*>;

    const Cells_t&      getCells() const  { return _cells; };
    const Borders_t&    getVerticalBorders() const  { return _verticalBorders; };
    const Borders_t&    getHorizontalBorders() const  { return _horizontalBorders; };
private:
    Cells_t     _cells;
    Borders_t   _verticalBorders;
    Borders_t   _horizontalBorders;
    //@}
    //-------------------------------------------------------------------------


    /*! \name Dragging Support Management *///---------------------------------
    //@{
public:
    /*! \brief Configure \c nodeItem in this group item (modify target item parenthcip, but keep same visual position).
     */
    virtual void    groupNodeItem(qan::NodeItem* nodeItem, qan::TableCell* groupCell = nullptr, bool transform = true) override;

    //! Configure \c nodeItem outside this group item (modify parentship, keep same visual position).
    virtual void    ungroupNodeItem(qan::NodeItem* nodeItem, bool transform = true) override;

protected:
    virtual void    mouseDoubleClickEvent(QMouseEvent* event ) override;
    virtual void    mousePressEvent(QMouseEvent* event ) override;
    //@}
    //-------------------------------------------------------------------------
};

} // ::qan

QML_DECLARE_TYPE(qan::TableGroupItem)
