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
// \file	qanNode.cpp
// \author	benoit@destrat.io
// \date	2004 February 15
//-----------------------------------------------------------------------------

// Qt headers
#include <QPainter>
#include <QPainterPath>

// QuickQanava headers
#include "./qanNode.h"
#include "./qanNodeItem.h"
#include "./qanGroup.h"
#include "./qanGraph.h"
#include "./qanTableCell.h"

namespace qan { // ::qan

/* Node Object Management *///-------------------------------------------------
Node::Node(QObject* parent) :
    super_t{parent}
{
    Q_UNUSED(parent)

    // Bind in/out nodes model lengthChanged() signal to in/ou degree modified signal.
    const auto inNodesModel = get_in_nodes().model();
    if (inNodesModel != nullptr)
        connect( inNodesModel, &qcm::ContainerModel::lengthChanged,
                 this,         &qan::Node::inDegreeChanged);
    const auto outNodesModel = get_out_nodes().model();
    if (outNodesModel != nullptr)
        connect( outNodesModel, &qcm::ContainerModel::lengthChanged,
                 this,          &qan::Node::outDegreeChanged);
}

Node::~Node()
{
    if (_item)
        _item->deleteLater();
}

qan::Graph*         Node::getGraph() noexcept { return get_graph(); }
const qan::Graph*   Node::getGraph() const noexcept { return get_graph(); }

bool    Node::operator==( const qan::Node& right ) const
{
    return getLabel() == right.getLabel();
}

qan::NodeItem*          Node::getItem() noexcept { return _item.data(); }
const qan::NodeItem*    Node::getItem() const noexcept { return _item.data(); }

void    Node::setItem(qan::NodeItem* nodeItem) noexcept
{
    if (nodeItem != nullptr) {
        _item = nodeItem;
        if (nodeItem->getNode() != this)
            nodeItem->setNode(this);
    }
}
//-----------------------------------------------------------------------------

/* Node Static Factories *///--------------------------------------------------
QQmlComponent*  Node::delegate(QQmlEngine& engine, QObject* parent) noexcept
{
    Q_UNUSED(parent)
    static std::unique_ptr<QQmlComponent>   delegate;
    if (!delegate)
        delegate = std::make_unique<QQmlComponent>(&engine, "qrc:/QuickQanava/Node.qml",
                                                   QQmlComponent::PreferSynchronous);
    return delegate.get();
}

qan::NodeStyle* Node::style(QObject* parent) noexcept
{
    static QScopedPointer<qan::NodeStyle>  qan_Node_style;
    if (!qan_Node_style)
        qan_Node_style.reset(new qan::NodeStyle(parent));
    return qan_Node_style.data();
}
//-----------------------------------------------------------------------------

/* Topology Interface *///-----------------------------------------------------
QAbstractItemModel* Node::qmlGetInNodes() const
{
    return const_cast<QAbstractItemModel*>(static_cast<const QAbstractItemModel*>(get_in_nodes().model()));
}

int     Node::getInDegree() const
{
    const auto model = get_in_nodes().model();
    return model != nullptr ? model->getLength() : -1;
}

QAbstractItemModel* Node::qmlGetOutNodes() const
{
    return const_cast< QAbstractItemModel* >(qobject_cast<const QAbstractItemModel*>(get_out_nodes().model()));
}

int     Node::getOutDegree() const
{
    const auto model = get_out_nodes().model();
    return model != nullptr ? model->getLength() : -1;
}

QAbstractItemModel* Node::qmlGetOutEdges() const
{
    return super_t::get_out_edges().model();
}

std::unordered_set<qan::Edge*>  Node::collectAdjacentEdges() const
{
    std::unordered_set<qan::Edge*> edges;
    for (const auto in_edge: qAsConst(get_in_edges())) {
        if (in_edge != nullptr)
            edges.insert(in_edge);
    }
    for (const auto out_edge: qAsConst(get_out_edges())) {
        if (out_edge != nullptr)
            edges.insert(out_edge);
    }
    return edges;
}
//-----------------------------------------------------------------------------

/* Behaviours Management *///--------------------------------------------------
void    Node::installBehaviour(std::unique_ptr<qan::NodeBehaviour> behaviour)
{
    // PRECONDITIONS:
        // behaviour can't be nullptr
    if (!behaviour)
        return;
    behaviour->setHost(this);
    add_node_observer(std::move(behaviour));
}
//-----------------------------------------------------------------------------

/* Appearance Management *///--------------------------------------------------
bool    Node::setLabel(const QString& label)
{
    if (label != _label) {
        _label = label;
        emit labelChanged();
        if (auto graph = getGraph())
            emit graph->nodeLabelChanged(this);
        return true;
    }
    return false;
}

bool    Node::setIsProtected(bool isProtected)
{
    if (isProtected != _isProtected) {
        _isProtected = isProtected;
        emit isProtectedChanged();
        return true;
    }
    return false;
}

bool    Node::setLocked(bool locked)
{
    if (locked != _locked) {
        _locked = locked;
        emit lockedChanged();
        return true;
    }
    return false;
}
//-----------------------------------------------------------------------------

/* Node Group Management *///--------------------------------------------------
bool    Node::setCell(qan::TableCell* cell)
{
    if (cell != _cell) {
        _cell = cell;
        emit cellChanged();
        return true;
    }
    return false;
}
//-----------------------------------------------------------------------------

} // ::qan
