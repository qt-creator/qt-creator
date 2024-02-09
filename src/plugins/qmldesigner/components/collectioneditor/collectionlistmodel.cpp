// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "collectionlistmodel.h"

#include "collectioneditorutils.h"

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

namespace {

template<typename ValueType>
bool containsItem(const std::initializer_list<ValueType> &container, const ValueType &value)
{
    auto begin = std::cbegin(container);
    auto end = std::cend(container);

    auto it = std::find(begin, end, value);
    return it != end;
}

} // namespace

namespace QmlDesigner {

CollectionListModel::CollectionListModel(const ModelNode &sourceModel)
    : QAbstractListModel()
    , m_sourceNode(sourceModel)
    , m_sourceType(CollectionEditorUtils::getSourceCollectionType(sourceModel))
{
    connect(this, &CollectionListModel::modelReset, this, &CollectionListModel::updateEmpty);
    connect(this, &CollectionListModel::rowsRemoved, this, &CollectionListModel::updateEmpty);
    connect(this, &CollectionListModel::rowsInserted, this, &CollectionListModel::updateEmpty);
}

QHash<int, QByteArray> CollectionListModel::roleNames() const
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

int CollectionListModel::rowCount([[maybe_unused]] const QModelIndex &parent) const
{
    return m_data.count();
}

bool CollectionListModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid())
        return false;

    if (containsItem<int>({Qt::EditRole, Qt::DisplayRole, NameRole}, role)) {
        if (contains(value.toString()))
            return false;

        QString oldName = collectionNameAt(index.row());
        bool nameChanged = value != data(index);
        if (nameChanged) {
            QString newName = value.toString();
            m_data.replace(index.row(), newName);
            emit dataChanged(index, index, {Qt::DisplayRole, Qt::EditRole});
            emit this->collectionNameChanged(oldName, newName);
        }
        return nameChanged;
    } else if (role == SelectedRole) {
        if (value.toBool() != index.data(SelectedRole).toBool()) {
            setSelectedIndex(value.toBool() ? index.row() : -1);
            return true;
        }
    }
    return false;
}

bool CollectionListModel::removeRows(int row, int count, const QModelIndex &parent)
{
    const int rows = rowCount(parent);
    if (row >= rows)
        return false;

    row = qBound(0, row, rows - 1);
    count = qBound(0, count, rows - row);

    if (count < 1)
        return false;

    QStringList removedCollections = m_data.mid(row, count);

    beginRemoveRows(parent, row, row + count - 1);
    m_data.remove(row, count);
    endRemoveRows();

    emit collectionsRemoved(removedCollections);
    if (m_selectedIndex >= row)
        selectCollectionIndex(m_selectedIndex - count, true);

    return true;
}

QVariant CollectionListModel::data(const QModelIndex &index, int role) const
{
    QTC_ASSERT(index.isValid(), return {});

    switch (role) {
    case IdRole:
        return index.row();
    case SelectedRole:
        return index.row() == m_selectedIndex;
    case NameRole:
    default:
        return m_data.at(index.row());
    }
}

void CollectionListModel::resetModelData(const QStringList &collectionsList)
{
    QString prevSelectedCollection = selectedIndex() > -1 ? m_data.at(selectedIndex()) : QString();
    beginResetModel();
    m_data = collectionsList;
    endResetModel();
    selectCollectionName(prevSelectedCollection);
}

int CollectionListModel::selectedIndex() const
{
    return m_selectedIndex;
}

ModelNode CollectionListModel::sourceNode() const
{
    return m_sourceNode;
}

QString CollectionListModel::sourceAddress() const
{
    return CollectionEditorUtils::getSourceCollectionPath(m_sourceNode);
}

bool CollectionListModel::contains(const QString &collectionName) const
{
    return m_data.contains(collectionName);
}

QStringList CollectionListModel::collections() const
{
    return m_data;
}

void CollectionListModel::selectCollectionIndex(int idx, bool selectAtLeastOne)
{
    int collectionCount = m_data.size();
    int preferredIndex = -1;
    if (collectionCount) {
        if (selectAtLeastOne)
            preferredIndex = std::max(0, std::min(idx, collectionCount - 1));
        else if (idx > -1 && idx < collectionCount)
            preferredIndex = idx;
    }

    setSelectedIndex(preferredIndex);
}

void CollectionListModel::selectCollectionName(const QString &collectionName)
{
    int idx = m_data.indexOf(collectionName);
    if (idx > -1)
        selectCollectionIndex(idx);
}

QString CollectionListModel::collectionNameAt(int idx) const
{
    return index(idx).data(NameRole).toString();
}

void CollectionListModel::addCollection(const QString &collectionName)
{
    if (m_data.contains(collectionName))
        return;

    int row = rowCount();
    beginInsertRows({}, row, row);
    m_data.append(collectionName);
    endInsertRows();

    emit collectionAdded(collectionName);
}

void CollectionListModel::setSelectedIndex(int idx)
{
    idx = (idx > -1 && idx < rowCount()) ? idx : -1;

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

void CollectionListModel::updateEmpty()
{
    bool isEmptyNow = m_data.isEmpty();
    if (m_isEmpty != isEmptyNow) {
        m_isEmpty = isEmptyNow;
        emit isEmptyChanged(m_isEmpty);

        if (m_isEmpty)
            setSelectedIndex(-1);
    }
}

} // namespace QmlDesigner
