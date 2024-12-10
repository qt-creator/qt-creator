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
// This file is a part of the QuickQanava library. Copyright 2016 Benoit AUTHEMAN.
//
// \file	qanLineGrid.h
// \author	benoit@destrat.io
// \date	2019 11 09 (initial code 2017 01 29)
//-----------------------------------------------------------------------------

#pragma once

// Qt headers
#include <QtQml>
#include <QQuickItem>
#include <QQmlListProperty>

// QuickQanava headers
#include "./qanGrid.h"

namespace qan {  // ::qan

/*! \brief Abstract grid with orthogonal geometry.
 *
 * \warning Changing \c geometryComponent dynamically is impossible and sill issue
 * a warning.
 *
 * \nosubgrouping
 */
class OrthoGrid : public Grid
{
    /*! \name OrthoGrid Object Management *///---------------------------------
    //@{
    Q_OBJECT
    QML_ELEMENT
public:
    explicit OrthoGrid(QQuickItem* parent = nullptr);
    virtual ~OrthoGrid() override = default;
    OrthoGrid(const OrthoGrid&) = delete;
    //@}
    //-------------------------------------------------------------------------

    /*! \name Grid Management *///---------------------------------------------
    //@{
public:
    virtual bool    updateGrid(const QRectF& viewRect,
                               const QQuickItem& container,
                               const QQuickItem& navigable) noexcept override;
protected:
    virtual bool    updateGrid() noexcept override;

private:
    //! View rect cached updated in updateGrid(), allow updateGrid() use with no arguments.
    QRectF                  _viewRectCache;
    //! Cache for container targetted by this grid, updated in updateGrid(), allow updateGrid() use with no arguments.
    QPointer<QQuickItem>    _containerCache;
    //! Cache for navigable where this grid is used, updated in updateGrid(), allow updateGrid() use with no arguments.
    QPointer<QQuickItem>    _navigableCache;
    //@}
    //-------------------------------------------------------------------------
};

namespace impl {  // ::qan::impl

/*! \brief Private utility class based on QObject to feed line to QML grid backend.
 */
class GridLine : public QObject
{
    Q_OBJECT
public:
    GridLine() = default;
    GridLine(QObject* parent) : QObject{parent} {}
    GridLine(const QPointF&& p1, const QPointF&& p2) :
        QObject{nullptr},
        _p1{p1}, _p2{p2} {}
    ~GridLine() = default;
    GridLine(const GridLine&) = delete;

public:
    Q_PROPERTY(QPointF p1 READ getP1 CONSTANT)
    Q_PROPERTY(QPointF p2 READ getP2 CONSTANT)

    const QPointF&  getP1() const { return _p1; }
    const QPointF&  getP2() const { return _p2; }
    QPointF&        getP1() { return _p1; }
    QPointF&        getP2() { return _p2; }
private:
    QPointF _p1;
    QPointF _p2;
};

} // ::qan::impl

/*! * \brief Draw an orthogonal grid with lines.
 *
 * \code
 *  Qan.Navigable {
 *    navigable: true
 *    Qan.LineGrid { id: lineGrid }
 *    grid: lineGrid
 *  }
 * \endcode
 *
 * \nosubgrouping
 */
class LineGrid : public OrthoGrid
{
    /*! \name LineGrid Object Management *///----------------------------------
    //@{
    Q_OBJECT
    QML_NAMED_ELEMENT(AbstractLineGrid)
public:
    explicit LineGrid( QQuickItem* parent = nullptr );
    virtual ~LineGrid() override;
    LineGrid(const LineGrid&) = delete;
    //@}
    //-------------------------------------------------------------------------

    /*! \name Grid Management *///---------------------------------------------
    //@{
signals:
    /*! \brief Emmitted when the grid's lines have to be redrawned.
     * \c minorLineToDrawCount and \c majorLineToDrawCount respectively the number of lines
     * to redraw.
     */
    void            redrawLines(int minorLineToDrawCount, int majorLineToDrawCount);

public:
    virtual bool    updateGrid(const QRectF& viewRect,
                               const QQuickItem& container,
                               const QQuickItem& navigable ) noexcept override;
public:
    Q_PROPERTY(QQmlListProperty<qan::impl::GridLine> minorLines READ getMinorLines CONSTANT)
    Q_PROPERTY(QQmlListProperty<qan::impl::GridLine> majorLines READ getMajorLines CONSTANT)

protected:
    QQmlListProperty<qan::impl::GridLine> getMinorLines();
    QQmlListProperty<qan::impl::GridLine> getMajorLines();

private:
    using size_type = qsizetype;

    size_type                   minorLinesCount() const;
    impl::GridLine*             minorLinesAt(size_type index) const;
    QVector<impl::GridLine*>    _minorLines;

    size_type                   majorLinesCount() const;
    impl::GridLine*             majorLinesAt(size_type index) const;
    QVector<impl::GridLine*>    _majorLines;

private:
    static size_type        callMinorLinesCount(QQmlListProperty<impl::GridLine>*);
    static impl::GridLine*  callMinorLinesAt(QQmlListProperty<impl::GridLine>*, size_type index);

    static size_type        callMajorLinesCount(QQmlListProperty<impl::GridLine>*);
    static impl::GridLine*  callMajorLinesAt(QQmlListProperty<impl::GridLine>*, size_type index);
    //@}
    //-------------------------------------------------------------------------
};

}  // ::qan

QML_DECLARE_TYPE(qan::OrthoGrid);
QML_DECLARE_TYPE(qan::impl::GridLine);
QML_DECLARE_TYPE(qan::LineGrid);
