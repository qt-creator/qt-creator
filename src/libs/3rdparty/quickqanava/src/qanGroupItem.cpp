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
// \file	qanGroupItem.cpp
// \author	benoit@destrat.io
// \date	2017 03 02
//-----------------------------------------------------------------------------

// QuickQanava headers
#include "./qanGraph.h"
#include "./qanGroupItem.h"
#include "./qanEdgeItem.h"
#include "./qanGroup.h"

namespace qan { // ::qan

/* Group Object Management *///------------------------------------------------
GroupItem::GroupItem(QQuickItem* parent) :
    qan::NodeItem{parent}
{
    qan::Draggable::configure(this);
    qan::Draggable::setAcceptDrops(true);

    setAcceptedMouseButtons(Qt::LeftButton | Qt::RightButton);

    // Force group connected edges update when the group is moved
    connect(this, &qan::GroupItem::xChanged,
            this, &qan::GroupItem::groupMoved);
    connect(this, &qan::GroupItem::yChanged,
            this, &qan::GroupItem::groupMoved);
    // Update adjacent edges z when group item z is modified.
    connect(this, &qan::GroupItem::zChanged,
            this, [this]() { this->groupMoved(); });

    connect(this, &qan::GroupItem::widthChanged,
            this, &qan::GroupItem::setDefaultBoundingShape);
    connect(this, &qan::GroupItem::heightChanged,
            this, &qan::GroupItem::setDefaultBoundingShape);

    setItemStyle(qan::Group::style(parent));
    setObjectName(QStringLiteral("qan::GroupItem"));
    // Note: Do not set width and height
}

auto    GroupItem::getGroup() noexcept -> qan::Group* { return _group.data(); }
auto    GroupItem::getGroup() const noexcept -> const qan::Group* { return _group.data(); }
bool    GroupItem::setGroup(qan::Group* group) noexcept
{
    // DraggableCtrl configuration is done in setNode()
    qan::NodeItem::setNode(static_cast<qan::Node*>(group));

    // Configuration specific to group
    if (group != _group) {
        _group = group;
        if (group != nullptr &&            // Warning: Do that after having set _group
            group->getItem() != this)
            group->setItem(this);
        return true;
    }
    return false;
}

auto    GroupItem::setRect(const QRectF& r) noexcept -> void
{
    // PRECONDITIONS:
        // r rect must be valid
    if (!r.isValid())
        return;
    setX(r.left());
    setY(r.top());
}
//-----------------------------------------------------------------------------


/* Collapse / Edition Management *///------------------------------------------
void    GroupItem::setCollapsed(bool collapsed) noexcept
{
    qan::NodeItem::setCollapsed(collapsed);
    // Note: Selection is hidden in base implementation
    if (_group) {
        const auto adjacentEdges = _group->collectAdjacentEdges();
        for (auto edge : adjacentEdges) {    // When a group is collapsed, all adjacent edges shouldbe hidden/shown...
            if (edge &&
                edge->getItem() != nullptr)
                edge->getItem()->setVisible(!getCollapsed());
        }
        if (!getCollapsed())
            groupMoved();   // Force update of all adjacent edges
    }
}

void    GroupItem::setExpandButtonVisible(bool expandButtonVisible) { _expandButtonVisible = expandButtonVisible; emit expandButtonVisibleChanged(); }
bool    GroupItem::getExpandButtonVisible() const { return _expandButtonVisible; }

void    GroupItem::setLabelEditorVisible(bool labelEditorVisible) { _labelEditorVisible = labelEditorVisible; emit labelEditorVisibleChanged(); }
bool    GroupItem::getLabelEditorVisible() const { return _labelEditorVisible; }
//-----------------------------------------------------------------------------

/* Group DnD Management *///---------------------------------------------------
bool GroupItem::setDragPolicy(DragPolicy dragPolicy) noexcept
{
    if (dragPolicy != _dragPolicy) {
        _dragPolicy = dragPolicy;
        return true;
    }
    return false;
}
GroupItem::DragPolicy          GroupItem::getDragPolicy() noexcept { return _dragPolicy; }
const GroupItem::DragPolicy    GroupItem::getDragPolicy() const noexcept { return _dragPolicy; }

void    GroupItem::groupMoved()
{
    if (getCollapsed())   // Do not update edges when the group is collapsed
        return;

    // Group node adjacent edges must be updated manually since node are children of this group,
    // their x an y position does not change and is no longer monitored by their edges.
    if (_group) {
        auto adjacentEdges = _group->collectAdjacentEdges();
        for (auto edge : adjacentEdges) {
            if (edge != nullptr &&
                edge->getItem() != nullptr)
                edge->getItem()->updateItem(); // Edge is updated even is edge item visible=false, updateItem() will take care of visibility
        }
    }
}

void    GroupItem::groupNodeItem(qan::NodeItem* nodeItem, qan::TableCell* groupCell, bool transform)
{
    // PRECONDITIONS:
        // nodeItem can't be nullptr
        // A 'container' must have been configured
    Q_UNUSED(groupCell)
    if (nodeItem == nullptr ||
        getContainer() == nullptr)   // A container must have configured in concrete QML group component
        return;

    // Note: no need for the container to be visible or open.
    if (transform) {
        const auto globalPos = nodeItem->mapToGlobal(QPointF{0., 0.});
        const auto groupPos = getContainer()->mapFromGlobal(globalPos);
        nodeItem->setPosition(groupPos);
    }
    nodeItem->setParentItem(getContainer());
    groupMoved();           // Force call to groupMoved() to update group adjacent edges
    endProposeNodeDrop();
}

void    GroupItem::ungroupNodeItem(qan::NodeItem* nodeItem, bool transform)
{
    if (nodeItem == nullptr)   // A container must have configured in concrete QML group component
        return;
    if (getGraph() &&
        getGraph()->getContainerItem() != nullptr) {
        const QPointF nodeGlobalPos = mapToItem(getGraph()->getContainerItem(), nodeItem->position());
        nodeItem->setParentItem(getGraph()->getContainerItem());
        if (transform)
            nodeItem->setPosition(nodeGlobalPos + QPointF{10., 10.});
        nodeItem->setZ(z() + 1.);
        nodeItem->setDraggable(true);
        nodeItem->setDroppable(true);
    }
}

bool    GroupItem::setContainer(QQuickItem* container) noexcept
{
    // PRECONDITIONS: None, container can be nullptr
    if (container != _container) {
        _container = container;
        emit containerChanged();
        return true;
    }
    return false;
}
QQuickItem*         GroupItem::getContainer() noexcept { return _container; }
const QQuickItem*   GroupItem::getContainer() const noexcept { return _container; }

void    GroupItem::setStrictDrop(bool strictDrop) noexcept
{
    if (strictDrop != _strictDrop) {
        _strictDrop = strictDrop;
        emit strictDropChanged();
    }
}
bool    GroupItem::getStrictDrop() const noexcept { return _strictDrop; }


void    GroupItem::mouseDoubleClickEvent(QMouseEvent* event)
{
    qan::NodeItem::mouseDoubleClickEvent(event);
    if (event->button() == Qt::LeftButton &&
        (getNode() != nullptr &&
         !getNode()->getLocked()))
        emit groupDoubleClicked(this, event->localPos());
}

void    GroupItem::mousePressEvent(QMouseEvent* event)
{
    qan::NodeItem::mousePressEvent(event);

    if ((event->button() == Qt::LeftButton ||
         event->button() == Qt::RightButton) &&    // Selection management
         getGroup() &&
         isSelectable() &&
         !getCollapsed() &&         // Locked/Collapsed group is not selectable
         !getNode()->getLocked()) {
        if (getGraph())
            getGraph()->selectGroup(*getGroup(), event->modifiers());
    }

    if (event->button() == Qt::LeftButton)
        emit groupClicked(this, event->localPos());
    else if (event->button() == Qt::RightButton)
        emit groupRightClicked(this, event->localPos());
}
//-----------------------------------------------------------------------------

} // ::qan
