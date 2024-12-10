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
// \file	qanDraggable.cpp
// \author	benoit@destrat.io
// \date	2016 03 15
//-----------------------------------------------------------------------------

// QuickQanava headers
#include "./qanDraggable.h"

namespace qan { // ::qan

/* Node Object Management *///-------------------------------------------------
void    Draggable::configure(QQuickItem* target)
{
    _target = target;
}
//-----------------------------------------------------------------------------

/* Draggable Management *///---------------------------------------------------
void    Draggable::setDraggable(bool draggable) noexcept
{
    if (draggable != _draggable) {
        _draggable = draggable;
        if (!draggable)
            setDragged(false);
        emitDraggableChanged();
    }
}

void    Draggable::setDragged(bool dragged) noexcept
{
    if (dragged != _dragged) {
        _dragged = dragged;
        emitDraggedChanged();
    }
}

void    Draggable::setDroppable(bool droppable) noexcept
{
    if (droppable != _droppable) {
        _droppable = droppable;
        emitDroppableChanged();
    }
}

void    Draggable::setAcceptDrops(bool acceptDrops) noexcept
{
    if (acceptDrops != _acceptDrops) {
        _acceptDrops = acceptDrops;
        if (acceptDrops &&
            _target &&
            !(_target->flags().testFlag(QQuickItem::ItemAcceptsDrops)))
            _target->setFlag( QQuickItem::ItemAcceptsDrops, acceptDrops);
        emitAcceptDropsChanged();
    }
}
//-----------------------------------------------------------------------------

} // ::qan
