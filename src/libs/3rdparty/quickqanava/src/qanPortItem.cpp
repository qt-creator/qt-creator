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
// \file	qanPortItem.cpp
// \author	benoit@destrat.io
// \date	2017 08 10
//-----------------------------------------------------------------------------

// Qt headers
#include <QPainter>
#include <QPainterPath>

// QuickQanava headers
#include "./qanPortItem.h"

namespace qan { // ::qan

/* Dock Object Management *///-------------------------------------------------
PortItem::PortItem(QQuickItem* parent) :
    qan::NodeItem{parent}
{
    setResizable(false);
    setDraggable(false);
    setSelectable(false);
    setObjectName(QStringLiteral("qan::PortItem"));

    setType(Type::InOut);
}
//-----------------------------------------------------------------------------

/* Port Properties Management *///---------------------------------------------
auto    PortItem::setMultiplicity(Multiplicity multiplicity) noexcept -> void
{
    if ( _multiplicity != multiplicity ) {
        _multiplicity = multiplicity;
        emit multiplicityChanged();
    }
}

auto    PortItem::setType(Type type) noexcept -> void
{
    _type = type;
    switch ( type ) {
    case Type::In:
        setConnectable(qan::NodeItem::Connectable::InConnectable);
        break;
    case Type::Out:
        setConnectable(qan::NodeItem::Connectable::OutConnectable);
        break;
    case Type::InOut:
        setConnectable(qan::NodeItem::Connectable::Connectable);
        break;
    };
}

void    PortItem::setDockType(NodeItem::Dock dockType) noexcept
{
    if (dockType != _dockType) {
        _dockType = dockType;
        emit dockTypeChanged();
    }
}

void    PortItem::setLabel(const QString& label) noexcept
{
    if (_label != label) {
        _label = label;
        emit labelChanged();
    }
}

void    PortItem::addInEdgeItem(qan::EdgeItem& inEdgeItem) noexcept
{
    QObject::connect(&inEdgeItem,   &QObject::destroyed,
                     this,          &qan::PortItem::onEdgeItemDestroyed);
    _inEdgeItems.append(&inEdgeItem);
}

void    PortItem::addOutEdgeItem(qan::EdgeItem& outEdgeItem) noexcept
{
    QObject::connect(&outEdgeItem,  &QObject::destroyed,
                     this,          &qan::PortItem::onEdgeItemDestroyed);
    _outEdgeItems.append(&outEdgeItem);
}

void    PortItem::onEdgeItemDestroyed(QObject* obj)
{
    // Connection to destroyed signal in addInEdgeItem() and addOutEdgeItem()
    const auto edgeItem = qobject_cast<qan::EdgeItem*>(obj);
    if (edgeItem != nullptr) {
        _inEdgeItems.removeAll(edgeItem);
        _outEdgeItems.removeAll(edgeItem);
    }
}

void    PortItem::updateEdges()
{
    for (auto inEdgeItem: _inEdgeItems)
        if (inEdgeItem != nullptr)
            inEdgeItem->updateItem();
    for (auto outEdgeItem: _outEdgeItems)
        if (outEdgeItem != nullptr)
            outEdgeItem->updateItem();
}
//-----------------------------------------------------------------------------

} // ::qan
