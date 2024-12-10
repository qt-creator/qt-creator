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
// \file	qanEdgeDraggableCtrl.cpp
// \author	benoit@destrat.io
// \date	2021 10 29
//-----------------------------------------------------------------------------

// QuickQanava headers
#include "./qanEdgeDraggableCtrl.h"
#include "./qanEdgeItem.h"
#include "./qanGraph.h"

namespace qan { // ::qan

qan::Graph*  EdgeDraggableCtrl::getGraph() noexcept
{
    return _targetItem ? _targetItem->getGraph() : nullptr;
}

/* Drag'nDrop Management *///--------------------------------------------------
bool    EdgeDraggableCtrl::handleDragEnterEvent(QDragEnterEvent* event) { Q_UNUSED(event); return false; }

void	EdgeDraggableCtrl::handleDragMoveEvent(QDragMoveEvent* event) { Q_UNUSED(event) }

void	EdgeDraggableCtrl::handleDragLeaveEvent(QDragLeaveEvent* event) { Q_UNUSED(event) }

void    EdgeDraggableCtrl::handleDropEvent(QDropEvent* event) { Q_UNUSED(event) }

void    EdgeDraggableCtrl::handleMouseDoubleClickEvent(QMouseEvent* event) { Q_UNUSED(event) }

bool    EdgeDraggableCtrl::handleMouseMoveEvent(QMouseEvent* event)
{
    // PRECONDITIONS:
        // graph must be non nullptr (configured).
        // _targetItem can't be nullptr
    if (!_targetItem)
        return false;
    const auto graph = _targetItem->getGraph();
    if (graph == nullptr)
        return false;

    // Early exits
    if (event->buttons().testFlag(Qt::NoButton))
        return false;
    if (!_targetItem->getDraggable())   // Do not drag if edge is non draggable
        return false;

    // Do not drag if edge source and destination nodes are locked.
    auto src = _targetItem->getSourceItem() != nullptr ? _targetItem->getSourceItem()->getNode() :
                                                         nullptr;
    auto dst = _targetItem->getDestinationItem() != nullptr ? _targetItem->getDestinationItem()->getNode() :
                                                              nullptr;
    if ((src && src->getLocked()) ||
        (src && src->getIsProtected()) ||
        (dst && dst->getLocked()) ||
        (dst && dst->getIsProtected()))
        return false;

    const auto rootItem = graph->getContainerItem();
    if (rootItem != nullptr &&      // Root item exist, left button is pressed and the target item
        event->buttons().testFlag(Qt::LeftButton)) {    // is draggable and not collapsed
        const auto sceneDragPos = rootItem->mapFromGlobal(event->globalPos());
        if (!_targetItem->getDragged()) {
            beginDragMove(sceneDragPos, _targetItem->getSelected());
            return true;
        } else {
            dragMove(sceneDragPos, _targetItem->getSelected());
            return true;
        }
    }
    return false;
}

void    EdgeDraggableCtrl::handleMousePressEvent(QMouseEvent* event) { Q_UNUSED(event) }

void    EdgeDraggableCtrl::handleMouseReleaseEvent(QMouseEvent* event)
{
    Q_UNUSED(event)
    if (_targetItem &&
        _targetItem->getDragged())
        endDragMove();
}

void    EdgeDraggableCtrl::beginDragMove(const QPointF& sceneDragPos, bool dragSelection, bool notify)
{
    Q_UNUSED(dragSelection)
    Q_UNUSED(notify)
    if (!_targetItem ||
        _target == nullptr)
        return;
    if (_target->getIsProtected() ||    // Prevent dragging of protected or locked objects
        _target->getLocked())
        return;
    //qWarning() << "EdgeDraggableCtrl::beginDragMove(): target=" << getTargetItem() << " dragSelection=" << dragSelection << " notify=" << notify;
    const auto graph = getGraph();
    if (graph == nullptr)
        return;

    // Get target edge adjacent nodes
    auto src = _targetItem->getSourceItem() != nullptr &&
                       _targetItem->getSourceItem()->getNode() != nullptr ? _targetItem->getSourceItem()->getNode()->getItem() :
                   nullptr;
    auto dst = _targetItem->getDestinationItem() != nullptr &&
                       _targetItem->getDestinationItem()->getNode() != nullptr ? _targetItem->getDestinationItem()->getNode()->getItem() :
                   nullptr;

    if (notify) {
        std::vector<qan::Node*> nodes;
        const auto& selectedNodes = graph->getSelectedNodes();
        std::copy(selectedNodes.begin(), selectedNodes.end(), std::back_inserter(nodes));
        std::copy(graph->getSelectedGroups().begin(), graph->getSelectedGroups().end(), std::back_inserter(nodes));
        nodes.push_back(src->getNode());
        nodes.push_back(dst->getNode());
        emit graph->nodesAboutToBeMoved(nodes);
    }
    _targetItem->setDragged(true);
    _initialDragPos = sceneDragPos;
    const auto rootItem = getGraph()->getContainerItem();
    if (rootItem != nullptr)   // Project in scene rect (for example is a node is part of a group)
        _initialTargetPos = rootItem->mapFromItem(_targetItem, QPointF{0,0});

    if (src != nullptr)
        src->draggableCtrl().beginDragMove(sceneDragPos, /*dragSelection=*/false, /*notify=*/false);
    if (dst != nullptr)
        dst->draggableCtrl().beginDragMove(sceneDragPos, /*dragSelection=*/false, /*notify=*/false);

    // If there is a selection, keep start position for all selected nodes.
    if (dragSelection &&
        graph->hasMultipleSelection()) {
        auto beginDragMoveSelected = [this, &sceneDragPos] (auto primitive) {    // Call beginDragMove() on a given node or group
            if (primitive != nullptr &&
                primitive->getItem() != nullptr &&
                static_cast<QQuickItem*>(primitive->getItem()) != static_cast<QQuickItem*>(this->_targetItem.data()))
                // Note 20231029: Set notify to false since notyification is done "once" with nodesaboutToBeMoved() in
                // the case of a multiple selection.
                primitive->getItem()->draggableCtrl().beginDragMove(sceneDragPos, /*dragSelection*/false, /*notify*/false);
        };
        // Call beginDragMove on all selection
        std::for_each(graph->getSelectedNodes().begin(), graph->getSelectedNodes().end(), beginDragMoveSelected);
        std::for_each(graph->getSelectedEdges().begin(), graph->getSelectedEdges().end(), beginDragMoveSelected);
        std::for_each(graph->getSelectedGroups().begin(), graph->getSelectedGroups().end(), beginDragMoveSelected);
    }
}

void    EdgeDraggableCtrl::dragMove(const QPointF& sceneDragPos, bool dragSelection,
                                    bool disableSnapToGrid, bool disableOrientation)
{
    //qWarning() << "EdgeDraggableCtrl::dragMove(): target=" << getTargetItem() << " dragSelection=" << dragSelection;
    Q_UNUSED(dragSelection)
    Q_UNUSED(disableSnapToGrid)
    Q_UNUSED(disableOrientation)
    // PRECONDITIONS:
        // _targetItem must be configured
    if (!_targetItem)
        return;
    if (_targetItem->getSourceItem() == nullptr ||
        _targetItem->getDestinationItem() == nullptr)
        return;

    // Get target edge adjacent nodes
    qan::Node* src = _targetItem->getSourceItem()->getNode();
    qan::Node* dst = _targetItem->getDestinationItem()->getNode();
    auto srcItem = src != nullptr ? src->getItem() : nullptr;
    auto dstItem = dst != nullptr ? dst->getItem() : nullptr;
    if (srcItem == nullptr ||
        dstItem == nullptr)
        return;

    // Polish snapToGrid:
    // When edge src|dst is not vertically or horizontally aligned: disable hook.
    // If they are vertically/horizontally aligned: allow move snapToGrid it won't generate jiterring.
    const auto disableHooksSnapToGrid = srcItem->getDragOrientation() == qan::NodeItem::DragOrientation::DragAll ||
                                        srcItem->getDragOrientation() == qan::NodeItem::DragOrientation::DragAll ||
                                        (srcItem->getDragOrientation() != dstItem->getDragOrientation());
    const auto disableHooksDragOrientation = (srcItem->getDragOrientation() == qan::NodeItem::DragOrientation::DragHorizontal ||
                                              srcItem->getDragOrientation() == qan::NodeItem::DragOrientation::DragVertical) &&
                                             (srcItem->getDragOrientation() == dstItem->getDragOrientation());
    if (srcItem != nullptr)
        srcItem->draggableCtrl().dragMove(sceneDragPos,
                                          /*dragSelection=*/false,
                                          /*disableSnapToGrid=*/disableHooksSnapToGrid,
                                          disableHooksDragOrientation);
    if (dstItem != nullptr)
        dstItem->draggableCtrl().dragMove(sceneDragPos,
                                          /*dragSelection=*/false,
                                          /*disableSnapToGrid=*/disableHooksSnapToGrid,
                                          disableHooksDragOrientation);

    // If there is a selection, keep start position for all selected nodes.
    const auto graph = getGraph();
    if (graph == nullptr)
        return;
    if (dragSelection) {
        auto dragMoveSelected = [this, &sceneDragPos] (auto primitive) { // Call dragMove() on a given node, group or edge
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
}

void    EdgeDraggableCtrl::endDragMove(bool dragSelection, bool notify)
{
    Q_UNUSED(dragSelection)
    Q_UNUSED(notify)
    if (!_targetItem)
        return;

    // FIXME #141 add an edgeMoved() signal...
    //emit graph->nodeMoved(_target);
    _targetItem->setDragged(false);

    // Get target edge adjacent nodes
    auto src = _targetItem->getSourceItem() != nullptr &&
               _targetItem->getSourceItem()->getNode() != nullptr ? _targetItem->getSourceItem()->getNode()->getItem() :
                                                                    nullptr;
    auto dst = _targetItem->getDestinationItem() != nullptr &&
               _targetItem->getDestinationItem()->getNode() != nullptr ? _targetItem->getDestinationItem()->getNode()->getItem() :
                                                                         nullptr;
    if (src != nullptr)
        src->draggableCtrl().endDragMove(/*dragSelection=*/false, /*notify=*/false);
    if (dst != nullptr)
        dst->draggableCtrl().endDragMove(/*dragSelection=*/false, /*notify=*/false);

    const auto graph = getGraph();
    if (graph == nullptr)
        return;
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

    if (notify) {   // With edge we _always_ move multiple nodes since there is at least src/dst
        std::vector<qan::Node*> nodes;
        nodes.push_back(src->getNode());
        nodes.push_back(dst->getNode());
        const auto& selectedNodes = graph->getSelectedNodes();
        std::copy(selectedNodes.begin(), selectedNodes.end(), std::back_inserter(nodes));
        std::copy(graph->getSelectedGroups().begin(), graph->getSelectedGroups().end(), std::back_inserter(nodes));
        emit graph->nodesMoved(nodes);
    }
}
//-----------------------------------------------------------------------------

} // ::qan
