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
// \file    qanTableGroup.h
// \author  benoit@destrat.io
// \date    2023 01 25
//-----------------------------------------------------------------------------

#pragma once

// Qt headers
#include <QQuickItem>
#include <QPointF>
#include <QPolygonF>

// QuickQanava headers
#include "./qanGroup.h"

Q_MOC_INCLUDE("./qanGraph.h")
Q_MOC_INCLUDE("./qanTableGroupItem.h")

namespace qan { // ::qan

/*! \brief Table is a specific group where nodes can be dropped inside rows en columns.
 *
 * \nosubgrouping
 */
class TableGroup : public qan::Group
{
    /*! \name TableGroup Object Management *///--------------------------------
    //@{
    Q_OBJECT
    QML_ELEMENT
public:
    //! TableGroup constructor.
    explicit TableGroup(QObject* parent = nullptr);
    explicit TableGroup(int cols, int rows);
    /*! \brief Remove any childs group who have no QQmlEngine::CppOwnership.
     *
     */
    virtual ~TableGroup() override = default;
    TableGroup(const TableGroup&) = delete;

public:
    //! \copydoc qan::Group::isTable()
    Q_INVOKABLE virtual bool    isTable() const override;

    //! Shortcut to getItem()->proposeNodeDrop(), defined only for g++ compatibility to avoid forward template declaration.
    void    itemProposeNodeDrop();
    //! Shortcut to getItem()->endProposeNodeDrop(), defined only for g++ compatibility to avoid forward template declaration.
    void    itemEndProposeNodeDrop();
    //@}
    //-------------------------------------------------------------------------

    /*! \name TableGroup Static Factories *///---------------------------------
    //@{
public:
    /*! \brief Return the default delegate QML component that should be used to generate group \c item.
     *
     *  \arg engine QML engine used for delegate QML component creation.
     *  \return Default delegate component or nullptr (when nullptr is returned, QuickQanava default to Qan.TableGroup component).
     */
    static  QQmlComponent*      delegate(QQmlEngine& engine, QObject* parent = nullptr) noexcept;

    /*! \brief Return the default style that should be used with qan::TableGroup.
     *
     *  \return Default style or nullptr (when nullptr is returned, qan::StyleManager default group style will be used).
     */
    static  qan::NodeStyle*     style(QObject* parent = nullptr) noexcept;
    //@}
    //-------------------------------------------------------------------------

    /*! \name Table Properties *///--------------------------------------------
    //@{
public:
    //! Override default qan::Group behaviour, a locked table group is no longer selectable / resizable.
    virtual bool    setLocked(bool locked) override;

    //! Initialize a "default" layout.
    Q_INVOKABLE void    initializeLayout();

public:
    //! \copydoc getRows()
    Q_PROPERTY(int rows READ getRows WRITE setRows NOTIFY rowsChanged FINAL)
    //! \copydoc getRows()
    bool        setRows(int rows);
    //! \brief Table row count.
    int         getRows() const { return _rows; }
signals:
    //! \copydoc getRows()
    void        rowsChanged();
private:
    //! \copydoc getRows()
    int         _rows = 3;

public:
    //! \copydoc getCols()
    Q_PROPERTY(int cols READ getCols WRITE setCols NOTIFY colsChanged FINAL)
    //! \copydoc getCols()
    bool        setCols(int cols);
    //! \brief Table column count.
    int         getCols() const { return _cols; }
signals:
    //! \copydoc getCols()
    void        colsChanged();
private:
    //! \copydoc getCols()
    int         _cols = 2;

public:
    //! \copydoc getCellSpacing()
    Q_PROPERTY(qreal cellSpacing READ getCellSpacing WRITE setCellSpacing NOTIFY cellSpacingChanged FINAL)
    //! \copydoc getCellSpacing()
    bool        setCellSpacing(qreal cellSpacing);
    //! \brief Spacing between table cells (padding is added on border cells).
    qreal       getCellSpacing() const { return _cellSpacing; }
signals:
    //! \copydoc getCellSpacing()
    void        cellSpacingChanged();
private:
    //! \copydoc getCellSpacing()
    qreal       _cellSpacing = 10.0;

public:
    //! \copydoc getCellTopPadding()
    Q_PROPERTY(qreal cellTopPadding READ getCellTopPadding WRITE setCellTopPadding NOTIFY cellTopPaddingChanged FINAL)
    //! \copydoc getCellTopPadding()
    bool        setCellTopPadding(qreal cellTopPadding);
    //! \brief Padding between cell content and cell top, might be usefull to "decorate" cell content.
    qreal       getCellTopPadding() const { return _cellTopPadding; }
signals:
    //! \copydoc getCellTopPadding()
    void        cellTopPaddingChanged();
private:
    //! \copydoc getCellTopPadding()
    qreal       _cellTopPadding = 0.0;

public:
    //! \brief FIXME #190.
    Q_PROPERTY(QSizeF cellMinimumSize READ getCellMinimumSize WRITE setCellMinimumSize NOTIFY cellMinimumSizeChanged FINAL)
    void        setCellMinimumSize(QSizeF cellMinimumSize);
    QSizeF      getCellMinimumSize() const { return _cellMinimumSize; }
signals:
    void        cellMinimumSizeChanged();
private:
    QSizeF      _cellMinimumSize = QSizeF{10., 10.};

public:
    //! \brief getTablePadding()
    Q_PROPERTY(qreal tablePadding READ getTablePadding WRITE setTablePadding NOTIFY tablePaddingChanged FINAL)
    //! \brief getTablePadding()
    bool        setTablePadding(qreal tablePadding);
    //! \brief Padding around table border and border cells.
    qreal       getTablePadding() const { return _tablePadding; }
signals:
    //! \brief getTablePadding()
    void        tablePaddingChanged();
private:
    //! \brief getTablePadding()
    qreal       _tablePadding = 5.0;
    //@}
    //-------------------------------------------------------------------------
};

} // ::qan

QML_DECLARE_TYPE(qan::TableGroup)
