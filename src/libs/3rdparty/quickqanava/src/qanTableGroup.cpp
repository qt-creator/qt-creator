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
// \file	qanTableGroup.cpp
// \author	benoit@destrat.io
// \date	2023 01 25
//-----------------------------------------------------------------------------

// Qt headers
#include <QBrush>
#include <QPainter>
#include <QPainterPath>

// QuickQanava headers
#include "./qanTableGroup.h"
#include "./qanTableGroupItem.h"

namespace qan { // ::qan

/* TableGroup Object Management *///-------------------------------------------
TableGroup::TableGroup(QObject* parent) :
    qan::Group{parent}
{
    set_is_group(true);
}

TableGroup::TableGroup(int cols, int rows) :
    qan::Group{nullptr},
    _rows{rows},
    _cols{cols}
{
    set_is_group(true);
}

bool    TableGroup::isTable() const { return true; }
//-----------------------------------------------------------------------------

/* TableGroup Static Factories *///--------------------------------------------
QQmlComponent*  TableGroup::delegate(QQmlEngine& engine, QObject* parent) noexcept
{
    static std::unique_ptr<QQmlComponent>   delegate;
    if (!delegate)
        delegate = std::make_unique<QQmlComponent>(&engine, "qrc:/QuickQanava/TableGroup.qml",
                                                   QQmlComponent::PreferSynchronous, parent);
    return delegate.get();
}

qan::NodeStyle* TableGroup::style(QObject* parent) noexcept
{
    static QScopedPointer<qan::NodeStyle>  qan_TableGroup_style;
    if (!qan_TableGroup_style) {
        qan_TableGroup_style.reset(new qan::NodeStyle{parent});
        qan_TableGroup_style->setFontPointSize(11);
        qan_TableGroup_style->setFontBold(true);
        qan_TableGroup_style->setLabelColor(QColor{"black"});
        qan_TableGroup_style->setBorderWidth(3.);
        qan_TableGroup_style->setBackRadius(8.);
        qan_TableGroup_style->setBackOpacity(0.90);
        qan_TableGroup_style->setBaseColor(QColor(240, 245, 250));
        qan_TableGroup_style->setBackColor(QColor(242, 248, 255));
    }
    return qan_TableGroup_style.get();
}
//-----------------------------------------------------------------------------

/* Table Properties *///-------------------------------------------------------
bool    TableGroup::setLocked(bool locked)
{
    if (qan::Group::setLocked(locked)) {
        // Note: dragging is automatically disable when target is
        // locked, see qan::DraggableCtrl::handleMouseMoveEvent()
        // Node resized signal will not be emmited since resizable is
        // set to false
        // Note: Nothing to do in fact !
        return true;
    }
    return false;
}

void    TableGroup::initializeLayout()
{
    auto tableGroupItem = qobject_cast<qan::TableGroupItem*>(getItem());
    if (tableGroupItem)
        tableGroupItem->initializeTableLayout();
}

bool    TableGroup::setRows(int rows)
{
    if (rows != _rows) {
        _rows = rows;
        emit rowsChanged();
        return true;
    }
    return false;
}

bool    TableGroup::setCols(int cols)
{
    if (cols != _cols) {
        _cols = cols;
        emit colsChanged();
        return true;
    }
    return false;
}

bool    TableGroup::setCellSpacing(qreal cellSpacing)
{
    if (!qFuzzyCompare(1. + cellSpacing, 1. + _cellSpacing)) {
        _cellSpacing = cellSpacing;
        emit cellSpacingChanged();
        return true;
    }
    return false;
}

bool    TableGroup::setCellTopPadding(qreal cellTopPadding)
{
    if (!qFuzzyCompare(1. + cellTopPadding, 1. + _cellTopPadding)) {
        _cellTopPadding = cellTopPadding;
        emit cellTopPaddingChanged();
        return true;
    }
    return false;
}

void    TableGroup::setCellMinimumSize(QSizeF cellMinimumSize)
{
    _cellMinimumSize = cellMinimumSize;
    emit cellMinimumSizeChanged();
}

bool    TableGroup::setTablePadding(qreal tablePadding)
{
    if (!qFuzzyCompare(1. + tablePadding, 1. + _tablePadding)) {
        _tablePadding = tablePadding;
        emit tablePaddingChanged();
        return true;
    }
    return false;
}
//-----------------------------------------------------------------------------

} // ::qan
