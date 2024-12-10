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
// \file	qanTableBorder.cpp
// \author	benoit@destrat.io
// \date	2023 01 26
//-----------------------------------------------------------------------------

// Std headers
#include <limits>
#include <algorithm>

// Qt headers
#include <QCursor>
#include <QMouseEvent>

// QuickQanava headers
#include "./qanTableBorder.h"
#include "./qanTableCell.h"
#include "./qanTableGroupItem.h"

namespace qan {  // ::qan

/* TableBorder Object Management *///-----------------------------------
TableBorder::TableBorder(QQuickItem* parent) :
    QQuickItem{parent}
{
    setAcceptedMouseButtons(Qt::LeftButton | Qt::RightButton);
    setAcceptHoverEvents(true);
}

TableBorder::~TableBorder() { /* prevCells and nextCells are not owned */ }
//-----------------------------------------------------------------------------

/* Border Management *///------------------------------------------------------
void    TableBorder::setTableGroup(qan::TableGroup* tableGroup)
{
    if (tableGroup != _tableGroup) {
        _tableGroup = tableGroup;
        emit tableGroupChanged();
    }
}

qreal   TableBorder::verticalCenter() const { return x() + (width() / 2.); }
qreal   TableBorder::horizontalCenter() const { return y() + (height() / 2.); }

qreal   TableBorder::getSx() const { return _sx; }
void    TableBorder::setSx(qreal sx)
{
    _sx = sx;
    emit sxChanged();
}

qreal   TableBorder::getSy() const { return _sy; }
void    TableBorder::setSy(qreal sy)
{
    _sy = sy;
    emit syChanged();
}

bool    TableBorder::setOrientation(Qt::Orientation orientation)
{
    if (orientation != _orientation) {
        _orientation = orientation;
        emit orientationChanged();
        return true;
    }
    return false;
}

void    TableBorder::setBorderColor(QColor borderColor)
{
    if (!borderColor.isValid())
        return;
    if (borderColor != _borderColor) {
        _borderColor = borderColor;
        emit borderColorChanged();
    }
}

void    TableBorder::setBorderWidth(qreal borderWidth)
{
    if (!qFuzzyCompare(1.0 + borderWidth, 1.0 + _borderWidth)) {
        _borderWidth = borderWidth;
        emit borderWidthChanged();
    }
}

void    TableBorder::setPrevBorder(qan::TableBorder* prevBorder)
{
    _prevBorder = prevBorder;
}
void    TableBorder::setNextBorder(qan::TableBorder* nextBorder)
{
    _nextBorder = nextBorder;
}

void    TableBorder::addPrevCell(qan::TableCell* prevCell)
{
    if (std::find(_prevCells.cbegin(), _prevCells.cend(), prevCell) != _prevCells.cend())
            return;
    if (prevCell != nullptr)
        _prevCells.push_back(prevCell);
}
void    TableBorder::addNextCell(qan::TableCell* nextCell)
{
    if (std::find(_nextCells.cbegin(), _nextCells.cend(), nextCell) != _nextCells.cend())
            return;
    if (nextCell != nullptr)
        _nextCells.push_back(nextCell);
}

void    TableBorder::onHorizontalMove() { layoutCells(); }
void    TableBorder::onVerticalMove() { layoutCells(); }

void    TableBorder::layoutCells()
{
    // qWarning() << "qan::TableBorder::layoutCells()";
    if (_tableGroup == nullptr)
        return;
    //qWarning() << "  _tableGroup.label=" << _tableGroup->getLabel();
    const auto tableGroupItem = qobject_cast<const qan::TableGroupItem*>(_tableGroup->getGroupItem());
    //if (tableGroupItem != nullptr)
    //    qWarning() << "  tableGroupItem=" << tableGroupItem << "  tableGroupItem.container=" << tableGroupItem->getContainer();
    if (tableGroupItem == nullptr ||
        tableGroupItem->getContainer() == nullptr)
        return;
    const auto padding = _tableGroup ? _tableGroup->getTablePadding() :
                                       2.;
    const auto spacing = _tableGroup ? _tableGroup->getCellSpacing() :
                                       5.;
    const auto spacing2 = spacing / 2.;
    const auto tableWidth = tableGroupItem->getContainer()->width();
    const auto tableHeight = tableGroupItem->getContainer()->height();
    //qWarning() << "  tableWidth=" << tableWidth << "  tableHeight=" << tableHeight;

    if (getOrientation() == Qt::Vertical) {
        const auto vCenter = verticalCenter();

        // Layout prev/next cells position and size
        for (auto prevCell: _prevCells) {
            if (prevCell == nullptr)
                continue;
            // qWarning() << "Border " << this;
            // qWarning() << "  border._prevBorder=" << _prevBorder;
            // qWarning() << "  border.vCenter=" << vCenter;
            if (!_prevBorder &&     // For first column, set cell x too (not prev border will do it).
                _tableGroup) {
                prevCell->setX(padding);
            }
            // qWarning() << "  prevCell.x=" << prevCell->x();
            const auto prevCellWidth = vCenter - prevCell->x() - spacing2;
            // qWarning() << "  prevCellWidth=" << prevCellWidth;
            prevCell->setWidth(prevCellWidth);
        }
        for (auto nextCell: _nextCells) {
            if (nextCell == nullptr)
                continue;
            nextCell->setX(vCenter + spacing2);
            if (_nextBorder &&
                _nextBorder->verticalCenter() > (x() + spacing))  // nextBorder might still not be initialized...
                nextCell->setWidth(_nextBorder->verticalCenter() - x() - spacing);

            if (!_nextBorder)    // For last column, set cell witdh too (no next border will do it).
                nextCell->setWidth(tableWidth - vCenter - padding - spacing2);
        }
    } // Vertical orientation
    else if (getOrientation() == Qt::Horizontal) {
        const auto hCenter = horizontalCenter();

        // Layout prev/next cells position and size
        for (auto prevCell: _prevCells) {
            if (prevCell == nullptr)
                continue;
            prevCell->setHeight(hCenter -
                                prevCell->y() - spacing2);
            if (!_prevBorder &&     // For first column, set cell x too (not prev border will do it).
                _tableGroup) {
                prevCell->setY(padding);
            }
        }
        for (auto nextCell: _nextCells) {
            if (nextCell == nullptr)
                continue;
            nextCell->setY(hCenter + spacing2);
            if (_nextBorder &&
                _nextBorder->horizontalCenter() > (y() + spacing))  // nextBorder might still not be initialized...
                nextCell->setHeight(_nextBorder->horizontalCenter() - y() - spacing);

            if (!_nextBorder)   // For last column, set cell witdh too (no next border will do it).
                nextCell->setHeight(tableHeight - hCenter - padding - spacing2);
        }
    }  // Horizontal orientation
}
//-----------------------------------------------------------------------------

/* Border Events Management *///-----------------------------------------------
void    TableBorder::hoverEnterEvent(QHoverEvent* event)
{
    if (_tableGroup &&
        !_tableGroup->getLocked() &&
        isVisible()) {
        if (getOrientation() == Qt::Vertical)
            setCursor(Qt::SplitHCursor);
        else if (getOrientation() == Qt::Horizontal)
                      setCursor(Qt::SplitVCursor);
        event->setAccepted(true);
    }
    else
        event->setAccepted(false);
}

void    TableBorder::hoverLeaveEvent(QHoverEvent* event)
{
    setCursor(Qt::ArrowCursor);
    event->setAccepted(true);
}

void    TableBorder::mouseMoveEvent(QMouseEvent* event)
{
    if (!_tableGroup ||
        _tableGroup->getLocked() ||
        !isVisible()) {
        event->setAccepted(false);
        return;
    }
    bool xModified = false;
    bool yModified = false;
    const auto mePos = event->scenePosition();
    if (event->buttons() |  Qt::LeftButton &&
        !_dragInitialMousePos.isNull()) {

        const QPointF startLocalPos = parentItem() != nullptr ? parentItem()->mapFromScene(_dragInitialMousePos) :
                                                                QPointF{.0, 0.};
        const QPointF curLocalPos = parentItem() != nullptr ? parentItem()->mapFromScene(mePos) :
                                                              QPointF{0., 0.};
        const QPointF delta{curLocalPos - startLocalPos};
        const auto position = _dragInitialPos + delta;

        // Bound move to ] prevCells.left(), nextCells.right() [
        const auto borderWidth2 = _borderWidth / 2.;
        const auto tableGroupItem = _tableGroup ? qobject_cast<const qan::TableGroupItem*>(_tableGroup->getGroupItem()) :
                                                  nullptr;
        if (tableGroupItem == nullptr ||
            tableGroupItem->getContainer() == nullptr)
            return;
        const auto tableSize = tableGroupItem->getContainer()->size();

        const auto padding = _tableGroup ? _tableGroup->getTablePadding() :
                                           2.;
        const auto spacing = _tableGroup ? _tableGroup->getCellSpacing() :
                                           5.;
        const auto spacing2 = spacing / 2.;

        if (getOrientation() == Qt::Vertical) {
            auto minX = std::numeric_limits<qreal>::max();
            if (_prevBorder == nullptr)     // Do not drag outside (on left) table group
                minX = padding + spacing2 + borderWidth2;
            else  {                         // Do not drag before prev border
                if (_prevBorder != nullptr)
                    minX = std::min(minX, _prevBorder->verticalCenter() + spacing2 + borderWidth2);
            }

            auto maxX = std::numeric_limits<qreal>::lowest();
            if (_nextBorder == nullptr)   // Do not drag outside (on right) table group
                maxX = tableSize.width() - padding - spacing2 - borderWidth2;
            else {                          // Do not drag after next border
                if (_nextBorder != nullptr)
                    maxX = std::max(maxX, _nextBorder->verticalCenter() - spacing - borderWidth2);
            }
            const auto x = qBound(minX, position.x(), maxX);
            setX(x);
            setSx(x / tableSize.width());
            xModified = true;
        }
        else if (getOrientation() == Qt::Horizontal) {
            auto minY = std::numeric_limits<qreal>::max();
            if (_prevBorder == nullptr)     // Do not drag outside (on top) table group
                minY = padding + spacing2 + borderWidth2;
            else  {                         // Do not drag before prev border
                if (_prevBorder != nullptr)
                    minY = std::min(minY, _prevBorder->horizontalCenter() + spacing2 + borderWidth2);
            }

            auto maxY = std::numeric_limits<qreal>::lowest();
            if (_nextBorder == nullptr)   // Do not drag outside (past/bottom) table group
                maxY = tableSize.height() - padding - spacing2 - borderWidth2;
            else {                          // Do not drag after/under next border
                if (_nextBorder != nullptr)
                    maxY = std::max(maxY, _nextBorder->horizontalCenter() - spacing - borderWidth2);
            }
            const auto y = qBound(minY, position.y(), maxY);
            setY(y);
            setSy(y / tableSize.height());
            yModified = true;
        }
        event->setAccepted(true);
    }
    if (xModified || yModified)
        layoutCells();
}

void    TableBorder::mousePressEvent(QMouseEvent* event)
{
    if (!_tableGroup ||
        _tableGroup->getLocked() ||
        !isVisible()) {
        event->setAccepted(false);
        return;
    }
    const auto mePos = event->scenePosition();
    _dragInitialMousePos = mePos;
    _dragInitialPos = position();
    event->setAccepted(true);
}

void    TableBorder::mouseReleaseEvent(QMouseEvent* event)
{
    Q_UNUSED(event)
    _dragInitialMousePos = {0., 0.};       // Invalid all cached coordinates when button is released
    _dragInitialPos = {0., 0.};
    emit modified();
}
//-------------------------------------------------------------------------

} // ::qan
