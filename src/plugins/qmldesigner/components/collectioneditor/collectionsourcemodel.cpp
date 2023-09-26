// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include "collectionsourcemodel.h"

#include "abstractview.h"
#include "variantproperty.h"

#include <utils/qtcassert.h>

namespace QmlDesigner {
CollectionSourceModel::CollectionSourceModel() {}

int CollectionSourceModel::rowCount(const QModelIndex &) const
{
    return m_collections.size();
}

QVariant CollectionSourceModel::data(const QModelIndex &index, int role) const
{
    QTC_ASSERT(index.isValid(), return {});

    const ModelNode *collection = &m_collections.at(index.row());

    switch (role) {
    case IdRole:
        return collection->id();
    case NameRole:
        return collection->variantProperty("objectName").value();
    case SelectedRole:
        return index.row() == m_selectedIndex;
    }

    return {};
}

bool CollectionSourceModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid())
        return false;

    ModelNode collection = m_collections.at(index.row());
    switch (role) {
    case IdRole: {
        if (collection.id() == value)
            return false;

        bool duplicatedId = Utils::anyOf(std::as_const(m_collections),
                                         [&collection, &value](const ModelNode &otherCollection) {
                                             return (otherCollection.id() == value
                                                     && otherCollection != collection);
                                         });
        if (duplicatedId)
            return false;

        collection.setIdWithRefactoring(value.toString());
    } break;
    case Qt::DisplayRole:
    case NameRole: {
        auto collectionName = collection.variantProperty("objectName");
        if (collectionName.value() == value)
            return false;

        collectionName.setValue(value.toString());
    } break;
    case SelectedRole: {
        if (value.toBool() != index.data(SelectedRole).toBool())
            setSelectedIndex(value.toBool() ? index.row() : -1);
        else
            return false;
    } break;
    default:
        return false;
    }

    return true;
}

bool CollectionSourceModel::removeRows(int row, int count, [[maybe_unused]] const QModelIndex &parent)
{
    const int rowMax = std::min(row + count, rowCount());

    if (row >= rowMax || row < 0)
        return false;

    AbstractView *view = m_collections.at(row).view();
    if (!view)
        return false;

    count = rowMax - row;

    bool selectionUpdateNeeded = m_selectedIndex >= row && m_selectedIndex < rowMax;

    // It's better to remove the group of nodes here because of the performance issue for the list,
    // and update issue for the view
    beginRemoveRows({}, row, rowMax - 1);

    view->executeInTransaction(Q_FUNC_INFO, [row, count, this]() {
        for (ModelNode node : Utils::span<const ModelNode>(m_collections).subspan(row, count)) {
            m_collectionsIndexHash.remove(node.internalId());
            node.destroy();
        }
    });

    m_collections.remove(row, count);

    int idx = row;
    for (const ModelNode &node : Utils::span<const ModelNode>(m_collections).subspan(row))
        m_collectionsIndexHash.insert(node.internalId(), ++idx);

    endRemoveRows();

    if (selectionUpdateNeeded)
        updateSelectedCollection();

    updateEmpty();
    return true;
}

QHash<int, QByteArray> CollectionSourceModel::roleNames() const
{
    static QHash<int, QByteArray> roles;
    if (roles.isEmpty()) {
        roles.insert(Super::roleNames());
        roles.insert({
            {IdRole, "collectionId"},
            {NameRole, "collectionName"},
            {SelectedRole, "collectionIsSelected"},
        });
    }
    return roles;
}

void CollectionSourceModel::setCollections(const ModelNodes &collections)
{
    beginResetModel();
    bool wasEmpty = isEmpty();
    m_collections = collections;
    m_collectionsIndexHash.clear();
    int i = 0;
    for (const ModelNode &collection : collections)
        m_collectionsIndexHash.insert(collection.internalId(), i++);

    if (wasEmpty != isEmpty())
        emit isEmptyChanged(isEmpty());

    endResetModel();

    updateSelectedCollection(true);
}

void CollectionSourceModel::removeCollection(const ModelNode &node)
{
    int nodePlace = m_collectionsIndexHash.value(node.internalId(), -1);
    if (nodePlace < 0)
        return;

    removeRow(nodePlace);
}

int CollectionSourceModel::collectionIndex(const ModelNode &node) const
{
    return m_collectionsIndexHash.value(node.internalId(), -1);
}

void CollectionSourceModel::selectCollection(const ModelNode &node)
{
    int nodePlace = m_collectionsIndexHash.value(node.internalId(), -1);
    if (nodePlace < 0)
        return;

    selectCollectionIndex(nodePlace, true);
}

QmlDesigner::ModelNode CollectionSourceModel::collectionNodeAt(int idx)
{
    QModelIndex data = index(idx);
    if (!data.isValid())
        return {};

    return m_collections.at(idx);
}

bool CollectionSourceModel::isEmpty() const
{
    return m_collections.isEmpty();
}

void CollectionSourceModel::selectCollectionIndex(int idx, bool selectAtLeastOne)
{
    int collectionCount = m_collections.size();
    int prefferedIndex = -1;
    if (collectionCount) {
        if (selectAtLeastOne)
            prefferedIndex = std::max(0, std::min(idx, collectionCount - 1));
        else if (idx > -1 && idx < collectionCount)
            prefferedIndex = idx;
    }

    setSelectedIndex(prefferedIndex);
}

void CollectionSourceModel::deselect()
{
    setSelectedIndex(-1);
}

void CollectionSourceModel::updateSelectedCollection(bool selectAtLeastOne)
{
    int idx = m_selectedIndex;
    m_selectedIndex = -1;
    selectCollectionIndex(idx, selectAtLeastOne);
}

void CollectionSourceModel::updateNodeName(const ModelNode &node)
{
    QModelIndex index = indexOfNode(node);
    emit dataChanged(index, index, {NameRole, Qt::DisplayRole});
}

void CollectionSourceModel::updateNodeId(const ModelNode &node)
{
    QModelIndex index = indexOfNode(node);
    emit dataChanged(index, index, {IdRole});
}

void CollectionSourceModel::setSelectedIndex(int idx)
{
    idx = (idx > -1 && idx < m_collections.count()) ? idx : -1;

    if (m_selectedIndex != idx) {
        QModelIndex previousIndex = index(m_selectedIndex);
        QModelIndex newIndex = index(idx);

        m_selectedIndex = idx;

        if (previousIndex.isValid())
            emit dataChanged(previousIndex, previousIndex, {SelectedRole});

        if (newIndex.isValid())
            emit dataChanged(newIndex, newIndex, {SelectedRole});

        emit selectedIndexChanged(idx);
    }
}

void CollectionSourceModel::updateEmpty()
{
    bool isEmptyNow = isEmpty();
    if (m_isEmpty != isEmptyNow) {
        m_isEmpty = isEmptyNow;
        emit isEmptyChanged(m_isEmpty);

        if (m_isEmpty)
            setSelectedIndex(-1);
    }
}

QModelIndex CollectionSourceModel::indexOfNode(const ModelNode &node) const
{
    return index(m_collectionsIndexHash.value(node.internalId(), -1));
}
} // namespace QmlDesigner
