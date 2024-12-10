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
// \file	qanDraggable.h
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

/*! \brief Interface for a draggable and droppable qan::Node or qan::Group.
 *
 * \nosubgrouping
 */
class Draggable
{
    /*! \name Draggable Object Management *///---------------------------------
    //@{
public:
    explicit Draggable() = default;
    virtual ~Draggable() = default;
    Draggable(const Draggable&) = delete;

protected:
    //! Configure this \c qan::Draggable interface with a valid \c target (usually a qan::Node or qan::Group).
    void    configure(QQuickItem* target);
private:
    QPointer<QQuickItem>    _target;
    QPointer<qan::Graph>    _graph;
    //@}
    //-------------------------------------------------------------------------

    /*! \name Draggable Management *///----------------------------------------
    //@{
public:
    void            setDraggable(bool draggable) noexcept;
    inline bool     getDraggable() const noexcept { return _draggable; }
protected:
    virtual void    emitDraggableChanged() = 0;
private:
    /*! \brief Define if the node could actually be dragged by mouse (default to true).
     *
     * Set this property to true if you want to allow this node to be moved by mouse (if false, the node position is
     * fixed and should be changed programmatically).
     *
     * Default to true.
     */
    bool            _draggable = true;

public:
    void            setDragged(bool dragged) noexcept;
    inline bool     getDragged() const noexcept { return _dragged; }
protected:
    virtual void    emitDraggedChanged() = 0;
private:
    //! True when the node is currently beeing dragged.
    bool            _dragged = false;

public:
    //! Set to false to prevent this primitive beeing dropped on groups (default to true ie droppable).
    void            setDroppable(bool droppable) noexcept;
    //! \copydoc setDroppable()
    inline bool     getDroppable() const noexcept { return _droppable; }
protected:
    virtual void    emitDroppableChanged() = 0;
private:
    /*! \brief Define if the target node or group could be dropped in another node or in a node group.
     *
     * Set this property to true if you want to allow this node to be dropped in a qan::Group automatically.
     * Default to true.
     * Setting this property to false may lead to a significant performance improvement if group dropping is not needed.
     */
    bool            _droppable = true;

public:
    void            setAcceptDrops(bool acceptDrops) noexcept;
    inline bool     getAcceptDrops() const noexcept { return _acceptDrops; }
protected:
    virtual void    emitAcceptDropsChanged() = 0;
private:
    /*! \brief Define if the group actually accept insertion of nodes via drag'n drop (default to true).
     *
     * Default to true.
     *
     * Setting this property to false may lead to a significant performance improvement if DropNode support is not needed.
     */
    bool            _acceptDrops = true;
    //@}
    //-------------------------------------------------------------------------
};

} // ::qan

Q_DECLARE_INTERFACE(
    qan::Draggable,
    "com.destrat.io.QuickQanava.Draggable/3.0"
)
