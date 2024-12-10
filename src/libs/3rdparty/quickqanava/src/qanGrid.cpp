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
// \file	qanGrid.cpp
// \author	benoit@destrat.io
// \date	2017 01 29
//-----------------------------------------------------------------------------

// Std headers
#include <math.h>        // std::round
#include <iostream>

// Qt headers

// QuickQanava headers
#include "./qanGrid.h"

namespace qan {  // ::qan

/* Grid Object Management *///-------------------------------------------------
Grid::Grid( QQuickItem* parent ) :
    QQuickItem{parent}
{
    setEnabled(false);  // Prevent event stealing, grids are not interactive
}

bool    Grid::updateGrid(const QRectF& viewRect,
                         const QQuickItem& container,
                         const QQuickItem& navigable ) noexcept
{
    Q_UNUSED(viewRect)
    Q_UNUSED(container)
    Q_UNUSED(navigable)
    return false;
}

bool    Grid::updateGrid() noexcept { return false; }
//-----------------------------------------------------------------------------

/* Grid Management *///--------------------------------------------------------
void    Grid::setThickColor( QColor thickColor ) noexcept
{
    if ( thickColor != _thickColor ) {
        _thickColor = thickColor;
        emit thickColorChanged();
    }
}

void    Grid::setGridWidth( qreal gridWidth ) noexcept
{
    if ( gridWidth < 0.001 ) {
        qWarning() << "qan::Grid::setMinorWidth(): Warning, major width should be superior to 0.0";
        return;
    }
    if ( !qFuzzyCompare(1.0 + gridWidth, 1.0 + _gridWidth) ) {
        _gridWidth = gridWidth;
        emit gridWidthChanged();
    }
}

void    Grid::setGridScale( qreal gridScale ) noexcept
{
    if ( gridScale < 0.001 ) {
        qWarning() << "qan::Grid::setGridScale(): Warning, grid scale should be superior to 0.0";
        return;
    }
    if ( !qFuzzyCompare(1.0 + gridScale, 1.0 + _gridScale) ) {
        _gridScale = gridScale;
        emit gridScaleChanged();
        updateGrid();
    }
}

void    Grid::setGridMajor( int gridMajor ) noexcept
{
    if ( gridMajor < 1 ) {
        qWarning() << "qan::Grid::setGridMajor(): Warning, grid major should be superior or equal to 1";
        return;
    }
    if ( gridMajor != _gridMajor ) {
        _gridMajor = gridMajor;
        emit gridMajorChanged();
        updateGrid();
    }
}
//-----------------------------------------------------------------------------

} // ::qan
