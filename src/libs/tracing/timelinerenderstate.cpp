// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "timelinerenderstate.h"
#include <utils/qtcassert.h>

namespace Timeline {

TimelineRenderState::TimelineRenderState(qint64 start, qint64 end, float scale, int numPasses) :
    m_expandedRowRoot(new QSGNode),
    m_collapsedRowRoot(new QSGNode),
    m_expandedOverlayRoot(new QSGNode),
    m_collapsedOverlayRoot(new QSGNode),
    m_start(start),
    m_end(end),
    m_scale(scale)
{
    m_passes.resize(numPasses);

    m_expandedRowRoot->setFlag(QSGNode::OwnedByParent, false);
    m_collapsedRowRoot->setFlag(QSGNode::OwnedByParent, false);
    m_expandedOverlayRoot->setFlag(QSGNode::OwnedByParent, false);
    m_collapsedOverlayRoot->setFlag(QSGNode::OwnedByParent, false);
}

TimelineRenderState::~TimelineRenderState()
{
    delete m_expandedRowRoot;
    delete m_collapsedRowRoot;
    delete m_expandedOverlayRoot;
    delete m_collapsedOverlayRoot;
    qDeleteAll(m_passes);
}

qint64 TimelineRenderState::start() const
{
    return m_start;
}

qint64 TimelineRenderState::end() const
{
    return m_end;
}

float TimelineRenderState::scale() const
{
    return m_scale;
}

const QSGNode *TimelineRenderState::expandedRowRoot() const
{
    return m_expandedRowRoot;
}

const QSGNode *TimelineRenderState::collapsedRowRoot() const
{
    return m_collapsedRowRoot;
}

const QSGNode *TimelineRenderState::expandedOverlayRoot() const
{
    return m_expandedOverlayRoot;
}

const QSGNode *TimelineRenderState::collapsedOverlayRoot() const
{
    return m_collapsedOverlayRoot;
}

QSGNode *TimelineRenderState::expandedRowRoot()
{
    return m_expandedRowRoot;
}

QSGNode *TimelineRenderState::collapsedRowRoot()
{
    return m_collapsedRowRoot;
}

QSGNode *TimelineRenderState::expandedOverlayRoot()
{
    return m_expandedOverlayRoot;
}

QSGNode *TimelineRenderState::collapsedOverlayRoot()
{
    return m_collapsedOverlayRoot;
}

bool TimelineRenderState::isEmpty() const
{
    return m_collapsedRowRoot->childCount() == 0 && m_expandedRowRoot->childCount() == 0 &&
            m_collapsedOverlayRoot->childCount() == 0 && m_expandedOverlayRoot->childCount() == 0;
}

void TimelineRenderState::assembleNodeTree(const TimelineModel *model, int defaultRowHeight,
                                           int defaultRowOffset)
{
    QTC_ASSERT(isEmpty(), return);

    for (int pass = 0; pass < m_passes.length(); ++pass) {
        const TimelineRenderPass::State *passState = m_passes[pass];
        if (!passState)
            continue;
        if (passState->expandedOverlay())
            m_expandedOverlayRoot->appendChildNode(passState->expandedOverlay());
        if (passState->collapsedOverlay())
            m_collapsedOverlayRoot->appendChildNode(passState->collapsedOverlay());
    }

    int row = 0;
    for (int i = 0; i < model->expandedRowCount(); ++i) {
        QSGTransformNode *rowNode = new QSGTransformNode;
        for (int pass = 0; pass < m_passes.length(); ++pass) {
            const TimelineRenderPass::State *passState = m_passes[pass];
            if (!passState)
                continue;
            const QList<QSGNode *> &rows = passState->expandedRows();
            if (rows.length() > row) {
                QSGNode *rowChildNode = rows[row];
                if (rowChildNode)
                    rowNode->appendChildNode(rowChildNode);
            }
        }
        m_expandedRowRoot->appendChildNode(rowNode);
        ++row;
    }

    for (int row = 0; row < model->collapsedRowCount(); ++row) {
        QSGTransformNode *rowNode = new QSGTransformNode;
        QMatrix4x4 matrix;
        matrix.translate(0, row * defaultRowOffset, 0);
        matrix.scale(1.0, static_cast<float>(defaultRowHeight) /
                     static_cast<float>(TimelineModel::defaultRowHeight()), 1.0);
        rowNode->setMatrix(matrix);
        for (int pass = 0; pass < m_passes.length(); ++pass) {
            const TimelineRenderPass::State *passState = m_passes[pass];
            if (!passState)
                continue;
            const QList<QSGNode *> &rows = passState->collapsedRows();
            if (rows.length() > row) {
                QSGNode *rowChildNode = rows[row];
                if (rowChildNode)
                    rowNode->appendChildNode(rowChildNode);
            }
        }
        m_collapsedRowRoot->appendChildNode(rowNode);
    }

    updateExpandedRowHeights(model, defaultRowHeight, defaultRowOffset);
}

void TimelineRenderState::updateExpandedRowHeights(const TimelineModel *model, int defaultRowHeight,
                                                   int defaultRowOffset)
{
    int row = 0;
    qreal offset = 0;
    for (QSGNode *rowNode = m_expandedRowRoot->firstChild(); rowNode != nullptr;
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
    QSGNode *rowNode = expanded ? m_expandedRowRoot : m_collapsedRowRoot;
    QSGNode *overlayNode = expanded ? m_expandedOverlayRoot : m_collapsedOverlayRoot;

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

TimelineRenderPass::State *TimelineRenderState::passState(int i)
{
    return m_passes[i];
}

const TimelineRenderPass::State *TimelineRenderState::passState(int i) const
{
    return m_passes[i];
}

void TimelineRenderState::setPassState(int i, TimelineRenderPass::State *state)
{
    m_passes[i] = state;
}

} // namespace Timeline
