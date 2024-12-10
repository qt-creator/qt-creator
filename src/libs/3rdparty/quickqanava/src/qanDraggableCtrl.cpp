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
// \file	qanDraggableCtrl.cpp
// \author	benoit@destrat.io
// \date	2017 03 15
//-----------------------------------------------------------------------------

// QuickQanava headers
#include "./qanDraggableCtrl.h"
#include "./qanNodeItem.h"
#include "./qanGraph.h"

namespace qan { // ::qan

/* Drag'nDrop Management *///--------------------------------------------------
bool    DraggableCtrl::handleDragEnterEvent(QDragEnterEvent* event)
{
    //qWarning() << "DraggableCtrl::handleDragEnterEvent(): this=" << this;
    //qWarning() << "event->source()=" << event->source();
    //qWarning() << "_targetItem->getAcceptDrops()=" << _targetItem->getAcceptDrops();

    if (_targetItem &&
        _targetItem->getAcceptDrops()) {
        if (event->source() == nullptr) {
            event->accept(); // This is propably a drag initated with type=Drag.Internal, for exemple a connector drop node, accept by default...
            return true;
        }
        else { // Get the source item from the quick drag attached object received
            QQuickItem* sourceItem = qobject_cast<QQuickItem*>(event->source());
            if (sourceItem != nullptr) {
                QVariant draggedStyle = sourceItem->property("draggedStyle"); // The source item (usually a style node or edge delegate must expose a draggedStyle property.
                if (draggedStyle.isValid()) {
                    event->accept();
                    return true;
                }
            }
        }
    }
    return false;
}

void	DraggableCtrl::handleDragMoveEvent(QDragMoveEvent* event)
{
    if (_targetItem &&
        _targetItem->getAcceptDrops()) {
        event->acceptProposedAction();
        event->accept();
    }
}

void	DraggableCtrl::handleDragLeaveEvent(QDragLeaveEvent* event)
{
    if (_targetItem &&
        _targetItem->getAcceptDrops())
        event->ignore();
}

void    DraggableCtrl::handleDropEvent(QDropEvent* event)
{
    if (_targetItem &&
        _targetItem->getAcceptDrops() &&
        event->source() != nullptr) { // Get the source item from the quick drag attached object received
        QQuickItem* sourceItem = qobject_cast<QQuickItem*>(event->source());
        if (sourceItem != nullptr) {
            QVariant draggedStyle = sourceItem->property("draggedStyle"); // The source item (usually a style node or edge delegate must expose a draggedStyle property).
            if (draggedStyle.isValid()) {
                auto style = draggedStyle.value<qan::Style*>();
                if (style != nullptr)
                    _targetItem->setItemStyle(style);
            }
        }
    }
}

void    DraggableCtrl::handleMouseDoubleClickEvent(QMouseEvent* event)
{
    Q_UNUSED(event)
}

bool    DraggableCtrl::handleMouseMoveEvent(QMouseEvent* event)
{
    // PRECONDITIONS:
        // graph must be non nullptr (configured).
        // _targetItem can't be nullptr
    const auto graph = getGraph();
    if (graph == nullptr)
        return false;
    if (!_targetItem)
        return false;

    // Early exits
    if (event->buttons().testFlag(Qt::NoButton))
        return false;
    if (!_targetItem->getDraggable())
        return false;
    if (_targetItem->getCollapsed())
        return false;

    if (_targetItem->getNode() != nullptr &&
            (_targetItem->getNode()->getLocked() ||
             _targetItem->getNode()->getIsProtected()))
        return false;

    const auto rootItem = getGraph()->getContainerItem();
    if (rootItem != nullptr &&      // Root item exist, left button is pressed and the target item
        event->buttons().testFlag(Qt::LeftButton)) {    // is draggable and not collapsed
        //const auto sceneDragPos = event->scenePosition();
        const auto sceneDragPos = graph->getContainerItem()->mapFromScene(event->scenePosition());
        if (!_targetItem->getDragged()) {
            // Project in scene rect (for example is a node is part of a group)
            beginDragMove(sceneDragPos, _targetItem->getSelected());
            return true;
        } else {
            dragMove(sceneDragPos, _targetItem->getSelected());
            return true;
        }
    }
    return false;
}

void    DraggableCtrl::handleMousePressEvent(QMouseEvent* event)
{
    Q_UNUSED(event)
}

void    DraggableCtrl::handleMouseReleaseEvent(QMouseEvent* event)
{
    Q_UNUSED(event)
    if (_targetItem &&
        _targetItem->getDragged())
        endDragMove();
}

void    DraggableCtrl::beginDragMove(const QPointF& sceneDragPos, bool dragSelection, bool notify)
{
    //qWarning() << "DraggableCtrl::beginDragMove(): sceneDragPos=" << sceneDragPos << " target=" << getTargetItem() << " dragSelection=" << dragSelection << " notify=" << notify;
    if (_targetItem == nullptr ||
        _target == nullptr)
        return;
    if (_target->getIsProtected() ||    // Prevent dragging of protected or locked objects
        _target->getLocked())
        return;
    const auto graph = getGraph();
    if (graph == nullptr)
        return;
    const auto graphContainerItem = graph->getContainerItem();
    if (graphContainerItem == nullptr)
        return;

    if (notify && _target->isGroup()) {
        const auto groupItem = qobject_cast<qan::GroupItem*>(_targetItem);
        const auto groupItemContainer = groupItem ? groupItem->getContainer() : nullptr;
        if (groupItem != nullptr && groupItemContainer != nullptr) {
            const auto groupItemDragPos = graphContainerItem->mapToItem(groupItemContainer, sceneDragPos);
            //qWarning() << "  groupItemDragPos=" << groupItemDragPos;
            bool drag = false;
            if ((groupItem->getDragPolicy() & qan::GroupItem::DragPolicy::Header) == qan::GroupItem::DragPolicy::Header) {
                if (groupItemDragPos.y() < 0)  // Coords are in container CS
                    drag = true;
            }
            if ((groupItem->getDragPolicy() & qan::GroupItem::DragPolicy::Container) == qan::GroupItem::DragPolicy::Container) {
                if (groupItemDragPos.y() >= 0)
                    drag = true;
            }
            if (!drag) {
                _targetItem->setDragged(false);
                return;
            }
        }
    }

    //qWarning() << "qan::DraggableCtrl::beginDragMove(): dragSelection=" << dragSelection;
    //qWarning() << "  graph->hasMultipleSelection()=" << graph->hasMultipleSelection();
    //qWarning() << "  notify=" << notify;
    // Note 20231029:
    // Notification is disabled when a multiple selection is dragged.
    // Use nodesAboutToBeMoved() route on multiple selection, nodeAboutToBeMoved() on single selection.
    if (notify) {
        std::vector<qan::Node*> nodes;
        const auto& selectedNodes = graph->getSelectedNodes();
        std::copy(selectedNodes.begin(), selectedNodes.end(), std::back_inserter(nodes));
        std::copy(graph->getSelectedGroups().begin(), graph->getSelectedGroups().end(), std::back_inserter(nodes));
        for (const auto& selectedEdge: graph->getSelectedEdges()) { // Add selected edges control points
            if (selectedEdge->getSource() != nullptr)
                nodes.push_back(selectedEdge->getSource());
            if (selectedEdge->getDestination() != nullptr)
                nodes.push_back(selectedEdge->getDestination());
        }
        if (selectedNodes.size() == 0)
            nodes.push_back(getTarget());
        if (dragSelection && graph->hasMultipleSelection()) {
            emit graph->nodesAboutToBeMoved(nodes);
        } else {
            for (const auto node: nodes)
                emit graph->nodeAboutToBeMoved(node);
        }
        //qWarning() << "  nodes.size()=" << nodes.size();
    }
    _targetItem->setDragged(true);

    _initialSceneDragPos = sceneDragPos;
    if (!_target->isGroup()) {
        _initialTargetZ = _targetItem->z();     // 20240821: Force maximum graph z when dragging a node (not a group), restored
        _targetItem->setZ(graph->getMaxZ() + 10.);    // in endDragMove
    }
    const auto rootItem = getGraph()->getContainerItem();
    if (rootItem != nullptr)   // Project in scene rect (for example if a node is part of a group)
        _initialTargetScenePos = rootItem->mapFromItem(_targetItem, QPointF{0,0});

    // If there is a selection, keep start position for all selected nodes.
    if (dragSelection &&
        graph->hasMultipleSelection()) {
            auto beginDragMoveSelected = [this, &sceneDragPos] (auto primitive) {    // Call beginDragMove() on a given node or group
                if (primitive != nullptr &&
                    primitive->getItem() != nullptr &&
                    static_cast<QQuickItem*>(primitive->getItem()) != static_cast<QQuickItem*>(this->_targetItem.data())/* &&
                    primitive->get_group() == nullptr*/)      // Do not drag nodes that are inside a group
                    // Note 20231029: Set notify to false since notyification is done "once" with nodesaboutToBeMoved() in
                    // the case of a multiple selection.
                    primitive->getItem()->draggableCtrl().beginDragMove(sceneDragPos, /*dragSelection*/false, /*notify*/false);
            };

        // Call beginDragMove on all selected nodes and groups.
        std::for_each(graph->getSelectedNodes().begin(), graph->getSelectedNodes().end(), beginDragMoveSelected);
        std::for_each(graph->getSelectedEdges().begin(), graph->getSelectedEdges().end(), beginDragMoveSelected);
        std::for_each(graph->getSelectedGroups().begin(), graph->getSelectedGroups().end(), beginDragMoveSelected);
    }
}

void    DraggableCtrl::dragMove(const QPointF& sceneDragPos, bool dragSelection,
                                bool disableSnapToGrid, bool disableOrientation)
{
    // PRECONDITIONS:
        // _graph must be configured (non nullptr)
        // _graph must have a container item for coordinate mapping
        // _target and _targetItem must be configured (true)
    //qWarning() << "DraggableCtrl::dragMove(): target=" << getTargetItem() << " dragSelection=" << dragSelection;

    if (!_target ||
        !_targetItem)
        return;
    if (_target->getIsProtected() ||    // Prevent dragging of protected or locked objects
        _target->getLocked())
        return;

    const auto graph = getGraph();
    if (graph == nullptr)
        return;
    const auto graphContainerItem = graph->getContainerItem();
    if (graphContainerItem == nullptr)
        return;

    // Algorithm:
    //   - Strategy:
    //     - both unsnap and snap final target positions are computed in global scene CS.
    //     - If the target is inside a group, positions are finally converted to it's parent CS
    //       - Target might be moved outside of a group during drag, it might be in a group
    //         before move and outside after move.

    // 1. Compute drag en position (ie original position + drag delta).
    // 2. Eventually, compute drag end position with snap to grid in scene CS.
    // 3. For grouped node, if end drag position is outside their actual group, ungroup node.
    // 4. Apply drag end position: convert from scene pos to an eventual parent group item CS.
    // 5. Propagate drag to other targets in selection.
    // 6. Eventually, propose a node group drop after move.

    // 1.
    const auto sceneDelta = (sceneDragPos - _initialSceneDragPos);    // _initialSceneDragPos is _always_ in scene CS.
    auto targetUnsnapScenePos = _initialTargetScenePos + sceneDelta;
    auto targetScenePos = targetUnsnapScenePos; // By default, do not snap

    // 2. Snap drag delta algorithm:
    // 2.1. Compute drag horisontal / verticall constrains
    // 2.2. If target position is "centered" on grid
    //    or mouse sceneDelta > grid
    // 2.3 Compute snapped position in scene coordinates
    const auto targetDragOrientation = _targetItem->getDragOrientation();
    const auto dragHorizontally = disableOrientation ||
                                  ((targetDragOrientation == qan::NodeItem::DragOrientation::DragAll) ||
                                   (targetDragOrientation == qan::NodeItem::DragOrientation::DragHorizontal));
    const auto dragVertically = disableOrientation ||
                                ((targetDragOrientation == qan::NodeItem::DragOrientation::DragAll) ||
                                 (targetDragOrientation == qan::NodeItem::DragOrientation::DragVertical));
    if (!disableSnapToGrid &&
        getGraph()->getSnapToGrid()) {
        const auto& gridSize = getGraph()->getSnapToGridSize();
        // 2.2
        // Smooth inital drag when object are not aligned on grid.
        bool applyX = dragHorizontally &&
                      std::fabs(sceneDelta.x()) > (gridSize.width() / 2.001);
        bool applyY = dragVertically &&
                      std::fabs(sceneDelta.y()) > (gridSize.height() / 2.001);
        /*if (!applyX && dragHorizontally) {
            const auto posModGridX = fmod(targetUnsnapScenePos.x(), gridSize.width());
            applyX = qFuzzyIsNull(posModGridX);
        }
        if (!applyY && dragVertically) {
            const auto posModGridY = fmod(targetUnsnapScenePos.y(), gridSize.height());
            applyY = qFuzzyIsNull(posModGridY);
        }*/
        // 2.3
        const auto targetSnapScenePosX = applyX ? gridSize.width() * std::round(targetUnsnapScenePos.x() / gridSize.width()) :
                                             _initialTargetScenePos.x();
        const auto targetSnapScenePosY = applyY ? gridSize.height() * std::round(targetUnsnapScenePos.y() / gridSize.height()) :
                                             _initialTargetScenePos.y();
        targetScenePos = QPointF{targetSnapScenePosX, targetSnapScenePosY};
    } else {
        // Apply drag constrain even if there is no snap to grid
        targetScenePos = QPointF{dragHorizontally ? targetUnsnapScenePos.x() : _initialTargetScenePos.x(),
                                 dragVertically ? targetUnsnapScenePos.y() : _initialTargetScenePos.y()
        };
    }

    // 3.
    auto targetGroupItem = _target->getGroup() != nullptr ? _target->getGroup()->getGroupItem() : nullptr;
    bool movedInsideGroup = false;
    if (targetGroupItem != nullptr) {
        // Convert the target "unsnap" position in group CS (ie it's parent CS)
        const auto targetUnsnapGroupPos = graphContainerItem->mapToItem(targetGroupItem, targetScenePos);
        const QRectF targetRect{
            targetUnsnapGroupPos,
            QSizeF{_targetItem->width(), _targetItem->height()}
        };
        const QRectF groupRect{
            QPointF{0., 0.},
            QSizeF{targetGroupItem->width(), targetGroupItem->height()}
        };
        movedInsideGroup = groupRect.contains(targetRect);
        if (!movedInsideGroup)
            graph->ungroupNode(_target, _target->get_group());
    }

    // 4. Apply drag end position
    // Refresh targetGroupItem it might have changed if target has been ungrouped
    targetGroupItem = _target->getGroup() != nullptr ? _target->getGroup()->getGroupItem() : nullptr;
    if (targetGroupItem) {
        const auto targetGroupPos = graphContainerItem->mapToItem(targetGroupItem, targetScenePos);
        _targetItem->setPosition(targetGroupPos);
    } else {
        _targetItem->setPosition(targetScenePos);
    }

    // 5.
    if (dragSelection) {
        auto dragMoveSelected = [this, &sceneDragPos] (auto primitive) { // Call dragMove() on a given node or group
            const auto primitiveIsNotSelf = static_cast<QQuickItem*>(primitive->getItem()) !=
                                            static_cast<QQuickItem*>(this->_targetItem.data());
            if (primitive != nullptr &&
                primitive->getItem() != nullptr &&
                primitiveIsNotSelf)       // Note: nodes inside a group or groups might be dragged too
                primitive->getItem()->draggableCtrl().dragMove(sceneDragPos, /*dragSelection=*/false);
        };

        std::for_each(graph->getSelectedNodes().begin(), graph->getSelectedNodes().end(), dragMoveSelected);
        std::for_each(graph->getSelectedEdges().begin(), graph->getSelectedEdges().end(), dragMoveSelected);
        std::for_each(graph->getSelectedGroups().begin(), graph->getSelectedGroups().end(), dragMoveSelected);
    }

    // 6. Eventually, propose a node group drop after move
    if (!movedInsideGroup &&
        _targetItem->getDroppable()) {
        qan::Group* group = graph->groupAt(_targetItem->mapToItem(graphContainerItem, QPointF{0., 0.}),
                                           {_targetItem->width(), _targetItem->height()},
                                           _targetItem /* except _targetItem */);
        if (group != nullptr &&
            !group->getLocked() &&
            group->getItem() != nullptr &&
            static_cast<QQuickItem*>(group->getItem()) != static_cast<QQuickItem*>(_targetItem.data()))  { // Do not drop a group in itself
            group->itemProposeNodeDrop();

            if (_lastProposedGroup &&              // When a node is already beeing proposed in a group (ie _lastProposedGroup is non nullptr), it
                _lastProposedGroup->getItem() != nullptr &&     // might also end up beeing dragged over a sub group of _lastProposedGroup, so
                group != _lastProposedGroup)                    // notify endProposeNodeDrop() for last proposed group
                _lastProposedGroup->itemEndProposeNodeDrop();
            _lastProposedGroup = group;
        } else if (group == nullptr &&
                   _lastProposedGroup != nullptr &&
                   _lastProposedGroup->getItem() != nullptr) {
            _lastProposedGroup->itemEndProposeNodeDrop();
            _lastProposedGroup = nullptr;
        }
    }
}

void    DraggableCtrl::endDragMove(bool dragSelection, bool notify)
{
    _initialSceneDragPos = QPointF{0., 0.};    // Invalid all cached coordinates when drag ends
    _initialTargetScenePos = QPointF{0., 0.};
    _lastProposedGroup = nullptr;

    // PRECONDITIONS:
        // _target and _targetItem must be configured (true)
        // _graph must be configured (non nullptr)
        // _graph must have a container item for coordinate mapping
    if (!_target ||
        !_targetItem)
        return;

    if (!_target->isGroup()) {
        _targetItem->setZ(_initialTargetZ);
        _initialTargetZ = 0.;
    }

    if (_target->getIsProtected() ||    // Prevent dragging of protected or locked objects
        _target->getLocked())
        return;

    const auto graph = getGraph();
    if (graph == nullptr)
        return;
    const auto graphContainerItem = graph->getContainerItem();
    if (graphContainerItem == nullptr)
        return;


    //qWarning() << "qan::DraggableCtrl::endDragMove(): dragSelection=" << dragSelection;
    //qWarning() << "  graph->hasMultipleSelection()=" << graph->hasMultipleSelection();
    //qWarning() << "  notify=" << notify;

    bool nodeGrouped = false;
    if (_targetItem->getDroppable()) {
        const auto targetScenePos = _targetItem->mapToItem(graphContainerItem, QPointF{0., 0.});
        qan::Group* group = graph->groupAt(targetScenePos, { _targetItem->width(), _targetItem->height() }, _targetItem);
        if (group != nullptr &&
            static_cast<QQuickItem*>(group->getItem()) != static_cast<QQuickItem*>(_targetItem.data())) { // Do not drop a group in itself
            if (group->getGroupItem() != nullptr &&             // Do not allow grouping a node in a collapsed
                !group->getGroupItem()->getCollapsed() &&       // or locked group item
                !group->getLocked()) {
                graph->groupNode(group, _target.data());
                nodeGrouped = true;
            }
        }
    }

    _targetItem->setDragged(false);

    if (dragSelection &&               // If there is a selection, end drag for the whole selection
        graph->hasMultipleSelection()) {
        auto enDragMoveSelected = [this] (auto primitive) { // Call dragMove() on a given node or group
            if ( primitive != nullptr &&
                 primitive->getItem() != nullptr &&
                 static_cast<QQuickItem*>(primitive->getItem()) != static_cast<QQuickItem*>(this->_targetItem.data()))
                primitive->getItem()->draggableCtrl().endDragMove(/*dragSelection*/false, /*notify*/false);
        };
        std::for_each(graph->getSelectedNodes().begin(), graph->getSelectedNodes().end(), enDragMoveSelected);
        std::for_each(graph->getSelectedEdges().begin(), graph->getSelectedEdges().end(), enDragMoveSelected);
        std::for_each(graph->getSelectedGroups().begin(), graph->getSelectedGroups().end(), enDragMoveSelected);
    }

    // Note 20231029:
    // Notification is disabled when a multiple selection is dragged.
    // Use nodesAboutToBeMoved() route on multiple selection, nodeAboutToBeMoved() on single selection.
    if (notify) {
        if (!graph->hasMultipleSelection() &&
            !nodeGrouped)  // Do not emit nodeMoved() if it has been grouped
            emit graph->nodeMoved(_target);
        else if (dragSelection &&
                 graph->hasMultipleSelection()) {
            std::vector<qan::Node*> nodes;
            const auto& selectedNodes = graph->getSelectedNodes();
            std::copy(selectedNodes.begin(), selectedNodes.end(), std::back_inserter(nodes));
            std::copy(graph->getSelectedGroups().begin(), graph->getSelectedGroups().end(), std::back_inserter(nodes));
            for (const auto& selectedEdge: graph->getSelectedEdges()) { // Add selected edges control points
                if (selectedEdge->getSource() != nullptr)
                    nodes.push_back(selectedEdge->getSource());
                if (selectedEdge->getDestination() != nullptr)
                    nodes.push_back(selectedEdge->getDestination());
            }
            emit graph->nodesMoved(nodes);
        }
    }
}
//-----------------------------------------------------------------------------

} // ::qan
