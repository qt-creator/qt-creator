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
// \file	qanLineGrid.cpp
// \author	benoit@destrat.io
// \date	2019 11 09 (initial code 2017 01 29)
//-----------------------------------------------------------------------------

// Std headers
#include <math.h>        // std::round
#include <iostream>

// Qt headers

// QuickQanava headers
#include "./qanLineGrid.h"

namespace qan {  // ::qan

/* OrthoGrid Object Management *///---------------------------------------------
OrthoGrid::OrthoGrid(QQuickItem* parent) :
    Grid{parent}
{ /* Nil */ }
//-----------------------------------------------------------------------------

/* Grid Management *///--------------------------------------------------------
bool    OrthoGrid::updateGrid(const QRectF& viewRect,
                              const QQuickItem& container,
                              const QQuickItem& navigable ) noexcept
{
    // PRECONDITIONS:
        // Grid item must be visible
        // View rect must be valid
        // _geometryComponent must have been set.
    if ( !isVisible() )   // Do not update an invisible Grid
        return false;
    if ( !viewRect.isValid() )
        return false;

    _viewRectCache = viewRect;      // Cache all arguments for an eventual updateGrid() call with
    _containerCache = const_cast<QQuickItem*>(&container);   // no arguments (for example on a qan::Grid property change)
    _navigableCache = const_cast<QQuickItem*>(&navigable);

    return true;
}

bool    OrthoGrid::updateGrid() noexcept
{
    if ( _containerCache &&
         _navigableCache &&
         _viewRectCache.isValid() ) // Update the grid with the last correct cached settings
        return updateGrid( _viewRectCache, *_containerCache, *_navigableCache );
    return false;
}
//-----------------------------------------------------------------------------


/* LineGrid Object Management *///---------------------------------------------
LineGrid::LineGrid(QQuickItem* parent) :
    OrthoGrid{parent} { /* Nil */ }

LineGrid::~LineGrid()
{
    for ( auto line : _minorLines ) {
        if ( line != nullptr &&
             QQmlEngine::objectOwnership(line) == QQmlEngine::CppOwnership )
             line->deleteLater();
    }
    _minorLines.clear();
    for ( auto line : _majorLines ) {
        if ( line != nullptr &&
             QQmlEngine::objectOwnership(line) == QQmlEngine::CppOwnership )
             line->deleteLater();
    }
    _majorLines.clear();
}
//-----------------------------------------------------------------------------

/* Grid Management *///--------------------------------------------------------
bool    LineGrid::updateGrid(const QRectF& viewRect,
                             const QQuickItem& container,
                             const QQuickItem& navigable) noexcept
{
    // PRECONDITIONS:
        // Base implementation should return true
        // gridShape property should have been set
    if (!OrthoGrid::updateGrid(viewRect, container, navigable))
        return false;

    const unsigned int  gridMajor{static_cast<unsigned int>(getGridMajor())}; // On major thick every 10 minor thicks
    const qreal gridScale{getGridScale()};

    qreal containerZoom = container.scale();
    if ( qFuzzyCompare(1.0 + containerZoom, 1.0) )  // Protect against 0 zoom
        containerZoom = 1.0;
    const qreal adaptativeScale = containerZoom < 1. ? gridScale / containerZoom : gridScale;
    QPointF rectifiedTopLeft{ std::floor(viewRect.topLeft().x() / adaptativeScale) * adaptativeScale,
                              std::floor(viewRect.topLeft().y() / adaptativeScale) * adaptativeScale };
    QPointF rectifiedBottomRight{ ( std::ceil(viewRect.bottomRight().x() / adaptativeScale) * adaptativeScale ),
                                  ( std::ceil(viewRect.bottomRight().y() / adaptativeScale) * adaptativeScale )};
    QRectF rectified{rectifiedTopLeft, rectifiedBottomRight };

    const int numLinesX = static_cast<int>(round(rectified.width() / adaptativeScale));
    const int numLinesY = static_cast<int>(round(rectified.height() / adaptativeScale));

    // Update points cache size
    const int linesCount = numLinesX + numLinesY;
    if ( _minorLines.size() < linesCount )
        _minorLines.resize(linesCount);
    if ( _minorLines.size() < linesCount )
        return false;
    if ( _majorLines.size() < linesCount )
        _majorLines.resize(linesCount);
    if ( _majorLines.size() < linesCount )
        return false;

    const auto navigableRectified = container.mapRectToItem(&navigable, rectified);
    int minorL = 0;
    int majorL = 0;
    for (int nlx = 0; nlx < numLinesX; ++nlx ){    // Generate HORIZONTAL lines
        auto lx = rectifiedTopLeft.x() + (nlx * adaptativeScale);
        auto navigablePoint = container.mapToItem(&navigable, QPointF{lx, 0.});

        const QPointF p1{navigablePoint.x(), navigableRectified.top()};
        const QPointF p2{navigablePoint.x(), navigableRectified.bottom()};

        const bool isMajorColumn = qFuzzyCompare( 1. + std::fmod(lx, gridMajor * adaptativeScale), 1. );
        if (isMajorColumn) {
            if (_majorLines[majorL] != nullptr ) {
                _majorLines[majorL]->getP1() = p1;
                _majorLines[majorL]->getP2() = p2;
            } else {
                impl::GridLine* line = new impl::GridLine{std::move(p1), std::move(p2)};
                _majorLines[majorL] = line;
            }
            ++majorL;
        } else {
            if (_minorLines[minorL] != nullptr ) {
                _minorLines[minorL]->getP1() = p1;
                _minorLines[minorL]->getP2() = p2;
            } else {
                impl::GridLine* line = new impl::GridLine{std::move(p1), std::move(p2)};
                _minorLines[minorL] = line;
            }
            ++minorL;
        }
    }
    for (int nly = 0; nly < numLinesY; ++nly) {    // Generate VERTICAL lines
        auto py = rectifiedTopLeft.y() + (nly * adaptativeScale);
        auto navigablePoint = container.mapToItem(&navigable, QPointF{0., py});

        const QPointF p1{navigableRectified.left(), navigablePoint.y()};
        const QPointF p2{navigableRectified.right(), navigablePoint.y()};

        const bool isMajorRow = qFuzzyCompare( 1. + std::fmod(py, gridMajor * adaptativeScale), 1. );
        if (isMajorRow) {
            if (_majorLines[majorL] == nullptr) {
                impl::GridLine* line = new impl::GridLine{std::move(p1), std::move(p2)};
                _majorLines[majorL] = line;
            } else {
                _majorLines[majorL]->getP1() = p1;
                _majorLines[majorL]->getP2() = p2;
            }
            ++majorL;
        } else {
            if (_minorLines[minorL] == nullptr) {
                impl::GridLine* line = new impl::GridLine{std::move(p1), std::move(p2)};
                _minorLines[minorL] = line;
            } else {
                _minorLines[minorL]->getP1() = p1;
                _minorLines[minorL]->getP2() = p2;
            }
            ++minorL;
        }
    }

    // Note: Lines when l < _lines.size() are left ignored, they won't be drawn since we
    // emit the number of lines to draw in redrawLines signal.
    emit redrawLines(minorL, majorL);
    return true;
}

QQmlListProperty<impl::GridLine> LineGrid::getMinorLines() {
    return QQmlListProperty<impl::GridLine>(this, this,
                                      &LineGrid::callMinorLinesCount,
                                      &LineGrid::callMinorLinesAt);
}
QQmlListProperty<impl::GridLine> LineGrid::getMajorLines() {
    return QQmlListProperty<impl::GridLine>(this, this,
                                      &LineGrid::callMajorLinesCount,
                                      &LineGrid::callMajorLinesAt);
}

LineGrid::size_type LineGrid::minorLinesCount() const { return _minorLines.size(); }
impl::GridLine*     LineGrid::minorLinesAt(size_type index) const { return _minorLines.at(index); }

LineGrid::size_type LineGrid::majorLinesCount() const { return _majorLines.size(); }
impl::GridLine* LineGrid::majorLinesAt(size_type index) const { return _majorLines.at(index); }

LineGrid::size_type LineGrid::callMinorLinesCount(QQmlListProperty<impl::GridLine>* list) { return reinterpret_cast<LineGrid*>(list->data)->minorLinesCount(); }
impl::GridLine*     LineGrid::callMinorLinesAt(QQmlListProperty<impl::GridLine>* list, size_type index) { return reinterpret_cast<LineGrid*>(list->data)->minorLinesAt(index); }

LineGrid::size_type LineGrid::callMajorLinesCount(QQmlListProperty<impl::GridLine>* list) { return reinterpret_cast<LineGrid*>(list->data)->majorLinesCount(); }
impl::GridLine*     LineGrid::callMajorLinesAt(QQmlListProperty<impl::GridLine>* list, size_type index) { return reinterpret_cast<LineGrid*>(list->data)->majorLinesAt(index); }
//-----------------------------------------------------------------------------

} // ::qan
