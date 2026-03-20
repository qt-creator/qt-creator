// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "timelinerenderstate.h"
#include <utils/qtcassert.h>

namespace Timeline {

TimelineRenderState::TimelineRenderState(qint64 start, qint64 end, float scale, int numPasses) :
    start(start),
    end(end),
    scale(scale)
{
    passes.resize(numPasses);

    expandedRowRoot.setFlag(QSGNode::OwnedByParent, false);
    collapsedRowRoot.setFlag(QSGNode::OwnedByParent, false);
    expandedOverlayRoot.setFlag(QSGNode::OwnedByParent, false);
    collapsedOverlayRoot.setFlag(QSGNode::OwnedByParent, false);
}

TimelineRenderState::~TimelineRenderState()
{
    qDeleteAll(passes);
}

bool TimelineRenderState::isEmpty() const
{
    return collapsedRowRoot.childCount() == 0 && expandedRowRoot.childCount() == 0 &&
            collapsedOverlayRoot.childCount() == 0 && expandedOverlayRoot.childCount() == 0;
}

void TimelineRenderState::assembleNodeTree(const TimelineModel *model, int defaultRowHeight,
                                           int defaultRowOffset)
{
    QTC_ASSERT(isEmpty(), return);

    for (int pass = 0; pass < passes.length(); ++pass) {
        const TimelineRenderPass::State *passState = passes[pass];
        if (!passState)
            continue;
        if (passState->expandedOverlay())
            expandedOverlayRoot.appendChildNode(passState->expandedOverlay());
        if (passState->collapsedOverlay())
            collapsedOverlayRoot.appendChildNode(passState->collapsedOverlay());
    }

    int row = 0;
    for (int i = 0; i < model->expandedRowCount(); ++i) {
        QSGTransformNode *rowNode = new QSGTransformNode;
        for (int pass = 0; pass < passes.length(); ++pass) {
            const TimelineRenderPass::State *passState = passes[pass];
            if (!passState)
                continue;
            const QList<QSGNode *> &rows = passState->expandedRows();
            if (rows.length() > row) {
                QSGNode *rowChildNode = rows[row];
                if (rowChildNode)
                    rowNode->appendChildNode(rowChildNode);
            }
        }
        expandedRowRoot.appendChildNode(rowNode);
        ++row;
    }

    for (int row = 0; row < model->collapsedRowCount(); ++row) {
        QSGTransformNode *rowNode = new QSGTransformNode;
        QMatrix4x4 matrix;
        matrix.translate(0, row * defaultRowOffset, 0);
        matrix.scale(1.0, static_cast<float>(defaultRowHeight) /
                     static_cast<float>(TimelineModel::defaultRowHeight()), 1.0);
        rowNode->setMatrix(matrix);
        for (int pass = 0; pass < passes.length(); ++pass) {
            const TimelineRenderPass::State *passState = passes[pass];
            if (!passState)
                continue;
            const QList<QSGNode *> &rows = passState->collapsedRows();
            if (rows.length() > row) {
                QSGNode *rowChildNode = rows[row];
                if (rowChildNode)
                    rowNode->appendChildNode(rowChildNode);
            }
        }
        collapsedRowRoot.appendChildNode(rowNode);
    }

    updateExpandedRowHeights(model, defaultRowHeight, defaultRowOffset);
}

void TimelineRenderState::updateExpandedRowHeights(const TimelineModel *model, int defaultRowHeight,
                                                   int defaultRowOffset)
{
    int row = 0;
    qreal offset = 0;
    for (QSGNode *rowNode = expandedRowRoot.firstChild(); rowNode != nullptr;
         rowNode = rowNode->nextSibling()) {
        qreal rowHeight = model->expandedRowHeight(row++);
        QMatrix4x4 matrix;
        matrix.translate(0, offset, 0);
        matrix.scale(1, rowHeight / defaultRowHeight, 1);
        offset += defaultRowOffset * rowHeight / defaultRowHeight;
        static_cast<QSGTransformNode *>(rowNode)->setMatrix(matrix);
    }
}

QSGTransformNode *TimelineRenderState::finalize(QSGNode *oldNode, bool expanded,
                                                const QMatrix4x4 &transform)
{
    QSGNode *rowNode = expanded ? &expandedRowRoot : &collapsedRowRoot;
    QSGNode *overlayNode = expanded ? &expandedOverlayRoot : &collapsedOverlayRoot;

    QSGTransformNode *node = oldNode ? static_cast<QSGTransformNode *>(oldNode) :
                                            new QSGTransformNode;
    node->setMatrix(transform);

    if (node->firstChild() != rowNode || node->lastChild() != overlayNode) {
        node->removeAllChildNodes();
        node->appendChildNode(rowNode);
        node->appendChildNode(overlayNode);
    }
    return node;
}

} // namespace Timeline
