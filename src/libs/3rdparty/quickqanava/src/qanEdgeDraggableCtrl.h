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
// \file	qanEdgeDraggableCtrl.h
// \author	benoit@destrat.io
// \date	2021 10 29
//-----------------------------------------------------------------------------

#pragma once

// Qt headers
#include <QQuickItem>
#include <QPointF>
#include <QPolygonF>
#include <QDrag>
#include <QPointer>

// QuickQanava headers
#include "./qanAbstractDraggableCtrl.h"
#include "./qanGroup.h"

namespace qan { // ::qan

/*! \brief Generic logic for dragging qan::Edge visual items.
 *
 * \nosubgrouping
 */
class EdgeDraggableCtrl : public qan::AbstractDraggableCtrl
{
    /*! \name Node Object Management *///--------------------------------------
    //@{
public:
    //! DraggableCtrl constructor.
    explicit EdgeDraggableCtrl() = default;
    virtual ~EdgeDraggableCtrl() override = default;
    EdgeDraggableCtrl(const EdgeDraggableCtrl&) = delete;

public:
    inline auto getTarget() noexcept -> qan::Edge* { return _target.data(); }
    inline auto getTarget() const noexcept -> const qan::Edge* { return _target.data(); }
    inline auto setTarget(qan::Edge* target) noexcept { _target = target; }
private:
    QPointer<qan::Edge>    _target{nullptr};

public:
    inline auto getTargetItem() noexcept -> qan::EdgeItem* { return _targetItem.data(); }
    inline auto getTargetItem() const noexcept -> const qan::EdgeItem* { return _targetItem.data(); }
    inline auto setTargetItem(qan::EdgeItem* targetItem) noexcept { _targetItem = targetItem; }
private:
    QPointer<qan::EdgeItem>    _targetItem;

protected:
    //! Utility to return a safe _targetItem->getGraph().
    qan::Graph*  getGraph() noexcept;
    //@}
    //-------------------------------------------------------------------------

    /*! \name Node DnD Management *///-----------------------------------------
    //@{
public:
    bool    handleDragEnterEvent(QDragEnterEvent* event);
    void    handleDragMoveEvent(QDragMoveEvent* event);
    void    handleDragLeaveEvent(QDragLeaveEvent* event);
    void    handleDropEvent(QDropEvent* event);

    void    handleMouseDoubleClickEvent(QMouseEvent* event);
    bool    handleMouseMoveEvent(QMouseEvent* event);
    void    handleMousePressEvent(QMouseEvent* event);
    void    handleMouseReleaseEvent(QMouseEvent* event);

public:
    //! \c dragInitialMousePos in window coordinate system.
    virtual void    beginDragMove(const QPointF& sceneDragPos, bool dragSelection = true, bool notify = true) override;
    //! \c delta in scene coordinate system.
    virtual void    dragMove(const QPointF& sceneDragPos, bool dragSelection = true,
                             bool disableSnapToGrid = false, bool disableOrientation = false) override;
    virtual void    endDragMove(bool dragSelection = true, bool notify = true) override;

private:
    //! Internal (mouse) initial dragging position.
    QPointF                 _initialDragPos{0., 0.};
    //! Internal (target) initial dragging position.
    QPointF                 _initialTargetPos{0., 0.};
    //@}
    //-------------------------------------------------------------------------
};

} // ::qan
