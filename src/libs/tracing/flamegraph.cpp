// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "flamegraph.h"

namespace FlameGraph {

FlameGraph::FlameGraph(QQuickItem *parent) :
    QQuickItem(parent)
{
    setAcceptedMouseButtons(Qt::LeftButton);

    // Queue the rebuild in this case so that a delegate can set the root without getting deleted
    // during the call.
    connect(this, &FlameGraph::rootChanged, this, &FlameGraph::rebuild, Qt::QueuedConnection);
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
    connect(m_model, &QAbstractItemModel::dataChanged,
            attached, &FlameGraphAttached::modelIndexChanged);
    m_delegate->completeCreate();
    return childObject;
}

int FlameGraph::buildLayout(const QModelIndex &parentIndex, int parentNodeIndex, int depth,
                            int maximumDepth)
{
    qreal position = 0;
    qreal skipped = 0;
    qreal parentSize = m_model->data(parentIndex, m_sizeRole).toReal();
    int rowCount = m_model->rowCount(parentIndex);
    int childrenDepth = depth;
    if (depth == maximumDepth - 1) {
        skipped = parentSize;
    } else {
        for (int row = 0; row < rowCount; ++row) {
            QModelIndex childIndex = m_model->index(row, 0, parentIndex);
            qreal size = m_model->data(childIndex, m_sizeRole).toReal();
            if (size / m_model->data(m_root, m_sizeRole).toReal() < m_sizeThreshold) {
                skipped += size;
                continue;
            }

            int nodeIndex = m_nodes.size();
            m_nodes.append({childIndex, position / parentSize, size / parentSize,
                            depth, parentNodeIndex});
            position += size;
            childrenDepth = qMax(childrenDepth,
                                 buildLayout(childIndex, nodeIndex, depth + 1, maximumDepth));
        }
    }

    // Invisible Root object: attribute all remaining width to "others"
    if (!parentIndex.isValid())
        skipped = parentSize - position;

    if (skipped > 0) {
        m_nodes.append({QModelIndex(), position / parentSize, skipped / parentSize,
                        depth, parentNodeIndex});
        childrenDepth = qMax(childrenDepth, depth + 1);
    }

    return childrenDepth;
}

void FlameGraph::buildItems()
{
    QQmlContext *context = qmlContext(this);
    QVector<QObject *> objects(m_nodes.size());
    for (int i = 0; i < m_nodes.size(); ++i) {
        const Node &node = m_nodes[i];
        QObject *parentObject = node.parentNodeIndex < 0
                ? static_cast<QObject *>(this)
                : objects[node.parentNodeIndex];
        QQuickItem *parentItem = qobject_cast<QQuickItem *>(parentObject);
        objects[i] = appendChild(parentObject, parentItem, context,
                                 node.modelIndex, node.relativePosition, node.relativeSize);
    }
}

void FlameGraph::rebuild()
{
    qDeleteAll(childItems());
    m_nodes.clear();
    m_depth = 0;

    if (!m_model) {
        emit depthChanged(m_depth);
        return;
    }

    if (m_model->data(m_root, m_sizeRole).toReal() > 0) {
        if (m_root.isValid()) {
            int rootNodeIndex = m_nodes.size();
            m_nodes.append({m_root, 0, 1, 0, -1});
            m_depth = buildLayout(m_root, rootNodeIndex, 1, m_maximumDepth);
        } else {
            m_depth = buildLayout(QModelIndex(), -1, 0, m_maximumDepth);
        }
        buildItems();
    }

    emit depthChanged(m_depth);
}

void FlameGraph::mousePressEvent(QMouseEvent *event)
{
    Q_UNUSED(event)
}

void FlameGraph::mouseReleaseEvent(QMouseEvent *event)
{
    Q_UNUSED(event)
    setSelectedTypeId(-1);
}

void FlameGraph::mouseDoubleClickEvent(QMouseEvent *event)
{
    Q_UNUSED(event)
    setSelectedTypeId(-1);
    resetRoot();
}

} // namespace FlameGraph
