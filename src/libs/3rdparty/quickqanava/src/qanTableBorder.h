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
// \file	qanTableBorder.h
// \author	benoit@destrat.io
// \date	2023 01 26
//-----------------------------------------------------------------------------

#pragma once

// Qt headers
#include <QtQml>
#include <QQuickItem>

#include "./qanTableGroup.h"

namespace qan {  // ::qan

class TableCell;

/*! \brief FIXME.
 *
 * \nosubgrouping
 */
class TableBorder : public QQuickItem
{
    /*! \name TableBorder Object Management *///------------------------
    //@{
    Q_OBJECT
    QML_NAMED_ELEMENT(AbstractTableBorder)
public:
    //! .
    explicit TableBorder(QQuickItem* parent = nullptr);
    virtual ~TableBorder() override;
    TableBorder(const TableBorder&) = delete;
    //@}
    //-------------------------------------------------------------------------

    /*! \name Border Management *///-------------------------------------------
    //@{
public:
    Q_PROPERTY(qan::TableGroup* tableGroup READ getTableGroup WRITE setTableGroup NOTIFY tableGroupChanged FINAL)
    qan::TableGroup*    getTableGroup() { return _tableGroup.data(); }
    void                setTableGroup(qan::TableGroup* tableGroup);
protected:
    QPointer<qan::TableGroup>     _tableGroup;
signals:
    void                tableGroupChanged();

public:
    //! Utility, return border vertical center, ie x() + width() / 2.
    qreal   verticalCenter() const;
    //! Utility, return border horizontal center, ie y() + height() / 2.
    qreal   horizontalCenter() const;

public:
    Q_PROPERTY(qreal sx READ getSx WRITE setSx NOTIFY sxChanged FINAL)
    qreal       getSx() const;
    void        setSx(qreal sX);
protected:
    qreal       _sx = 0.;
signals:
    void        sxChanged();

public:
    Q_PROPERTY(qreal sy READ getSy WRITE setSy NOTIFY syChanged FINAL)
    qreal       getSy() const;
    void        setSy(qreal sY);
protected:
    qreal       _sy = 0.;
signals:
    void        syChanged();

public:
    Q_PROPERTY(Qt::Orientation orientation READ getOrientation WRITE setOrientation NOTIFY orientationChanged FINAL)
    Qt::Orientation getOrientation() const { return _orientation; }
    bool            setOrientation(Qt::Orientation orientation);
protected:
    Qt::Orientation _orientation = Qt::Vertical;
signals:
    void            orientationChanged();

public:
    /*! \brief Border line color.
     */
    Q_PROPERTY(QColor borderColor READ getBorderColor WRITE setBorderColor NOTIFY borderColorChanged FINAL)
    void        setBorderColor(QColor borderColor);
    QColor      getBorderColor() const { return _borderColor; }
signals:
    void        borderColorChanged();
private:
    QColor      _borderColor{Qt::black};

public:
    /*! \brief Radius of the bottom right border rectangle component (default to \c 4).
     */
    Q_PROPERTY(qreal borderWidth READ getBorderWidth WRITE setBorderWidth NOTIFY borderWidthChanged FINAL)
    void        setBorderWidth(qreal borderWidth);
    qreal       getBorderWidth() const { return _borderWidth; }
signals:
    void        borderWidthChanged();
private:
    qreal       _borderWidth = 3.0;

public:
    void        setPrevBorder(qan::TableBorder* prevBorder);
    void        setNextBorder(qan::TableBorder* nextBorder);
protected:
    QPointer<qan::TableBorder>  _prevBorder;
    QPointer<qan::TableBorder>  _nextBorder;

public:
    using Cells_t = std::vector<qan::TableCell*>;
    void            addPrevCell(qan::TableCell* prevCell);
    void            addNextCell(qan::TableCell* nextCell);
    const Cells_t&  getPrevCells() const { return _prevCells; };
    const Cells_t&  getNextCells() const { return _nextCells; };
protected:
    Cells_t         _prevCells;
    Cells_t         _nextCells;

protected slots:
    void    onHorizontalMove();
    void    onVerticalMove();

public:
    //! Layout prev/next border cells.
    void    layoutCells();
    //@}
    //-------------------------------------------------------------------------

    /*! \name Border Events Management *///------------------------------------
    //@{
protected:
    virtual void    hoverEnterEvent(QHoverEvent *event) override;
    virtual void    hoverLeaveEvent(QHoverEvent *event) override;
    virtual void    mouseMoveEvent(QMouseEvent* event) override;
    virtual void    mousePressEvent(QMouseEvent* event) override;
    virtual void    mouseReleaseEvent(QMouseEvent* event) override;
private:
    //! Initial global mouse position at the beginning of a resizing border drag.
    QPointF         _dragInitialMousePos{0., 0.};
    QPointF         _dragInitialPos{0., 0.};

signals:
    //! Emitted when (after) the border position has been modified.
    void            modified();
    //@}
    //-------------------------------------------------------------------------
};

}  // ::qan

QML_DECLARE_TYPE(qan::TableBorder);
