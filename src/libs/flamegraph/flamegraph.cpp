/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "flamegraph.h"

namespace FlameGraph {

FlameGraph::FlameGraph(QQuickItem *parent) :
    QQuickItem(parent)
{
}

QQmlComponent *FlameGraph::delegate() const
{
    return m_delegate;
}

void FlameGraph::setDelegate(QQmlComponent *delegate)
{
    if (delegate != m_delegate) {
        m_delegate = delegate;
        emit delegateChanged(delegate);
    }
}

QAbstractItemModel *FlameGraph::model() const
{
    return m_model;
}

void FlameGraph::setModel(QAbstractItemModel *model)
{
    if (model != m_model) {
        if (m_model)
            disconnect(m_model, &QAbstractItemModel::modelReset, this, &FlameGraph::rebuild);

        m_model = model;
        if (m_model)
            connect(m_model, &QAbstractItemModel::modelReset, this, &FlameGraph::rebuild);
        emit modelChanged(model);
        rebuild();
    }
}

int FlameGraph::sizeRole() const
{
    return m_sizeRole;
}

void FlameGraph::setSizeRole(int sizeRole)
{
    if (sizeRole != m_sizeRole) {
        m_sizeRole = sizeRole;
        emit sizeRoleChanged(sizeRole);
        rebuild();
    }
}

qreal FlameGraph::sizeThreshold() const
{
    return m_sizeThreshold;
}

void FlameGraph::setSizeThreshold(qreal sizeThreshold)
{
    if (sizeThreshold != m_sizeThreshold) {
        m_sizeThreshold = sizeThreshold;
        emit sizeThresholdChanged(sizeThreshold);
        rebuild();
    }
}

int FlameGraph::depth() const
{
    return m_depth;
}

FlameGraphAttached *FlameGraph::qmlAttachedProperties(QObject *object)
{
    FlameGraphAttached *attached =
            object->findChild<FlameGraphAttached *>(QString(), Qt::FindDirectChildrenOnly);
    if (!attached)
        attached = new FlameGraphAttached(object);
    return attached;
}

QObject *FlameGraph::appendChild(QObject *parentObject, QQuickItem *parentItem,
                                 QQmlContext *context, const QModelIndex &childIndex,
                                 qreal position, qreal size)
{
    QObject *childObject = m_delegate->beginCreate(context);
    if (parentItem) {
        QQuickItem *childItem = qobject_cast<QQuickItem *>(childObject);
        if (childItem)
            childItem->setParentItem(parentItem);
    }
    childObject->setParent(parentObject);
    FlameGraphAttached *attached = FlameGraph::qmlAttachedProperties(childObject);
    attached->setRelativePosition(position);
    attached->setRelativeSize(size);
    attached->setModelIndex(childIndex);
    m_delegate->completeCreate();
    return childObject;
}


int FlameGraph::buildNode(const QModelIndex &parentIndex, QObject *parentObject, int depth,
                          int maximumDepth)
{
    qreal position = 0;
    qreal skipped = 0;
    qreal parentSize = m_model->data(parentIndex, m_sizeRole).toReal();
    QQuickItem *parentItem = qobject_cast<QQuickItem *>(parentObject);
    QQmlContext *context = qmlContext(this);
    int rowCount = m_model->rowCount(parentIndex);
    int childrenDepth = depth;
    if (depth == maximumDepth - 1) {
        skipped = parentSize;
    } else {
        for (int row = 0; row < rowCount; ++row) {
            QModelIndex childIndex = m_model->index(row, 0, parentIndex);
            qreal size = m_model->data(childIndex, m_sizeRole).toReal();
            if (size / m_model->data(QModelIndex(), m_sizeRole).toReal() < m_sizeThreshold) {
                skipped += size;
                continue;
            }

            QObject *childObject = appendChild(parentObject, parentItem, context, childIndex,
                                               position / parentSize, size / parentSize);
            position += size;
            childrenDepth = qMax(childrenDepth, buildNode(childIndex, childObject, depth + 1,
                                                          maximumDepth));
        }
    }

    // Root object: attribute all remaining width to "others"
    if (!parentIndex.isValid())
        skipped = parentSize - position;

    if (skipped > 0) {
        appendChild(parentObject, parentItem, context, QModelIndex(), position / parentSize,
                    skipped / parentSize);
        childrenDepth = qMax(childrenDepth, depth + 1);
    }

    return childrenDepth;
}

void FlameGraph::rebuild()
{
    qDeleteAll(childItems());
    childItems().clear();
    m_depth = 0;

    if (!m_model) {
        emit depthChanged(m_depth);
        return;
    }

    m_depth = buildNode(QModelIndex(), this, 0, m_maximumDepth);
    emit depthChanged(m_depth);
}

} // namespace FlameGraph
