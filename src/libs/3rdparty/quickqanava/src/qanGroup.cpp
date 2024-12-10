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
// \file	qanGroup.cpp
// \author	benoit@destrat.io
// \date	2016 03 22
//-----------------------------------------------------------------------------

// Qt headers
#include <QBrush>
#include <QPainter>
#include <QPainterPath>

// QuickQanava headers
#include "./qanNode.h"
#include "./qanGroup.h"
#include "./qanGroupItem.h"
#include "./qanGraph.h"

namespace qan { // ::qan

/* Group Object Management *///------------------------------------------------
Group::Group(QObject* parent) :
    qan::Node{parent}
{
    set_is_group(true);
}

bool    Group::isTable() const { return false; }

std::unordered_set<qan::Edge*>  Group::collectAdjacentEdges() const
{
    std::unordered_set<qan::Edge*> edges = qan::Node::collectAdjacentEdges();
    if (is_group()) {
        for (const auto groupNode: qAsConst(group_nodes())) {
            if (groupNode != nullptr) {
                const auto qanGroupNode = qobject_cast<qan::Group*>(groupNode);
                if (qanGroupNode != nullptr) {
                    const auto groupNodeEdges = qanGroupNode->collectAdjacentEdges();
                    edges.insert(groupNodeEdges.begin(),
                                 groupNodeEdges.end());
                } else {
                    auto qanNode = qobject_cast<qan::Node*>(groupNode);
                    if (qanNode != nullptr) {
                        auto nodeEdges = qanNode->collectAdjacentEdges();
                        edges.insert(nodeEdges.begin(), nodeEdges.end());
                    }
                }
            }
        }
    }
    return edges;
}

qan::GroupItem* Group::getGroupItem() noexcept { return qobject_cast<qan::GroupItem*>(getItem()); }
const qan::GroupItem*   Group::getGroupItem() const noexcept { return qobject_cast<const qan::GroupItem*>(getItem()); }

void    Group::setItem(qan::NodeItem* item) noexcept
{
    qan::Node::setItem(item);
    const auto groupItem = qobject_cast<qan::GroupItem*>(item);
    if (groupItem != nullptr) {
        if (groupItem->getGroup() != this)
            groupItem->setGroup(this);
    }
}

void    Group::itemProposeNodeDrop()
{
    const auto groupItem = getGroupItem();
    if (groupItem)
        groupItem->proposeNodeDrop();
}

void    Group::itemEndProposeNodeDrop()
{
    const auto groupItem = getGroupItem();
    if (groupItem)
        groupItem->endProposeNodeDrop();
}
//-----------------------------------------------------------------------------

/* Group Static Factories *///-------------------------------------------------
QQmlComponent*  Group::delegate(QQmlEngine& engine, QObject* parent) noexcept
{
    static std::unique_ptr<QQmlComponent>   delegate;
    if (!delegate)
        delegate = std::make_unique<QQmlComponent>(&engine, "qrc:/QuickQanava/Group.qml",
                                                   QQmlComponent::PreferSynchronous, parent);
    return delegate.get();
}

qan::NodeStyle* Group::style(QObject* parent) noexcept
{
    static QScopedPointer<qan::NodeStyle>  qan_Group_style;
    if (!qan_Group_style) {
        qan_Group_style.reset(new qan::NodeStyle{parent});
        qan_Group_style->setFontPointSize(11);
        qan_Group_style->setFontBold(true);
        qan_Group_style->setLabelColor(QColor{"black"});
        qan_Group_style->setBorderWidth(2.);
        qan_Group_style->setBackRadius(8.);
        qan_Group_style->setBackOpacity(0.90);
        qan_Group_style->setBaseColor(QColor(240, 245, 250));
        qan_Group_style->setBackColor(QColor(242, 248, 255));
    }
    return qan_Group_style.get();
}
//-----------------------------------------------------------------------------

/* Group Nodes Management *///-------------------------------------------------
bool    Group::hasNode(const qan::Node* node) const { return qan::Node::has_node(node); }
//-----------------------------------------------------------------------------

} // ::qan
