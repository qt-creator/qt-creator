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
// \file	qanSelectable.h
// \author	benoit@destrat.io
// \date	2017 03 15
//-----------------------------------------------------------------------------

#pragma once

// Qt headers
#include <QObject>      // Q_DECLARE_INTERFACE
#include <QPointer>
#include <QQuickItem>

namespace qan { // ::qan

class Graph;

/*! \brief Interface for a selectable qan::Node or qan::Group.
 *
 * \nosubgrouping
 */
class Selectable
{
    /*! \name Selectable Object Management *///--------------------------------
    //@{
public:
    explicit Selectable();
    virtual ~Selectable();
    Selectable(const Selectable&) = delete;

protected:
    //! Configure this \c qan::Selectable interface with a valid graph.
    void    configure(QQuickItem* target, qan::Graph* graph);
private:
    QPointer<QQuickItem>    _target;
    QPointer<qan::Graph>    _graph;
    //@}
    //-------------------------------------------------------------------------

    /*! \name Selection Management *///----------------------------------------
    //@{
public:
    void            setSelectable(bool selectable) noexcept;
    inline bool     getSelectable() const noexcept { return _selectable; }
    inline bool     isSelectable() const noexcept { return _selectable; }
protected:
    virtual void    emitSelectableChanged() = 0;
private:
    bool            _selectable = true;

public:
    void            setSelected(bool selected) noexcept;
    inline bool     getSelected() const noexcept { return _selected; }
protected:
    virtual void    emitSelectedChanged() = 0;
private:
    bool            _selected = false;

public:
    /*! \brief Item used to hilight selection.
     *
     * \note Selection items are ususally created in qan::Graph::createSelectionItem() using
     * the currently registered \c selectionDelegate set in qan::Graph.
    */
    inline QQuickItem*  getSelectionItem() noexcept { return _selectionItem.data(); }
    //! \copydoc getSelectionItem()
    void                setSelectionItem(QQuickItem* selectionItem) noexcept;
protected:
    //! \copydoc getSelectionItem()
    virtual void        emitSelectionItemChanged() = 0;
private:
    //! \copydoc getSelectionItem()
    QPointer<QQuickItem>  _selectionItem{nullptr};

public:
    /*! \brief Configure the minimum needed properties of selectionItem to show it properly.
     *
     * \note Selection items are usually created using createSelectionItem(). A custom
     * QML delegate should support QuickQanava selection item properties otherwise setting selection
     * properties (such as color, weight or margin) at graph level won't work. No warnings or errors are
     * generated when a custom delegate does not support this interface and it is totally valid to use
     * a custom delegate with no global selection properties support.
     */
    void            configureSelectionItem();
};

} // ::qan

Q_DECLARE_INTERFACE(
    qan::Selectable,
    "com.destrat.io.QuickQanava.Selectable/3.0"
)
