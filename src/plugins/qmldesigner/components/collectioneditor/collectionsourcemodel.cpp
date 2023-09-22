// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "collectionsourcemodel.h"

#include "abstractview.h"
#include "collectioneditorconstants.h"
#include "collectionlistmodel.h"
#include "variantproperty.h"

#include <utils/qtcassert.h>

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>

namespace {

QSharedPointer<QmlDesigner::CollectionListModel> loadCollection(
    const QmlDesigner::ModelNode &sourceNode,
    QSharedPointer<QmlDesigner::CollectionListModel> initialCollection = {})
{
    using namespace QmlDesigner::CollectionEditor;
    QString sourceFileAddress = sourceNode.variantProperty(SOURCEFILE_PROPERTY).value().toString();

    QSharedPointer<QmlDesigner::CollectionListModel> collectionsList;
    auto setupCollectionList = [&sourceNode, &initialCollection, &collectionsList]() {
        if (initialCollection.isNull())
            collectionsList.reset(new QmlDesigner::CollectionListModel(sourceNode));
        else if (initialCollection->sourceNode() == sourceNode)
            collectionsList = initialCollection;
        else
            collectionsList.reset(new QmlDesigner::CollectionListModel(sourceNode));
    };

    if (sourceNode.type() == JSONCOLLECTIONMODEL_TYPENAME) {
        QFile sourceFile(sourceFileAddress);
        if (!sourceFile.open(QFile::ReadOnly))
            return {};

        QJsonParseError parseError;
        QJsonDocument document = QJsonDocument::fromJson(sourceFile.readAll(), &parseError);
        if (parseError.error != QJsonParseError::NoError)
            return {};

        setupCollectionList();

        if (document.isObject()) {
            const QJsonObject sourceObject = document.object();
            collectionsList->setStringList(sourceObject.toVariantMap().keys());
        }
    } else if (sourceNode.type() == CSVCOLLECTIONMODEL_TYPENAME) {
        QmlDesigner::VariantProperty collectionNameProperty = sourceNode.variantProperty(
            "objectName");
        setupCollectionList();
        collectionsList->setStringList({collectionNameProperty.value().toString()});
    }
    return collectionsList;
}
} // namespace

namespace QmlDesigner {

CollectionSourceModel::CollectionSourceModel() {}

int CollectionSourceModel::rowCount(const QModelIndex &) const
{
    return m_collectionSources.size();
}

QVariant CollectionSourceModel::data(const QModelIndex &index, int role) const
{
    QTC_ASSERT(index.isValid(), return {});

    const ModelNode *collectionSource = &m_collectionSources.at(index.row());

    switch (role) {
    case IdRole:
        return collectionSource->id();
    case NameRole:
        return collectionSource->variantProperty("objectName").value();
    case SourceRole:
        return collectionSource->variantProperty(CollectionEditor::SOURCEFILE_PROPERTY).value();
    case SelectedRole:
        return index.row() == m_selectedIndex;
    case CollectionsRole:
        return QVariant::fromValue(m_collectionList.at(index.row()).data());
    }

    return {};
}

bool CollectionSourceModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid())
        return false;

    ModelNode collectionSource = m_collectionSources.at(index.row());
    switch (role) {
    case IdRole: {
        if (collectionSource.id() == value)
            return false;

        bool duplicatedId = Utils::anyOf(std::as_const(m_collectionSources),
                                         [&collectionSource, &value](const ModelNode &otherCollection) {
                                             return (otherCollection.id() == value
                                                     && otherCollection != collectionSource);
                                         });
        if (duplicatedId)
            return false;

        collectionSource.setIdWithRefactoring(value.toString());
    } break;
    case Qt::DisplayRole:
    case NameRole: {
        auto collectionName = collectionSource.variantProperty("objectName");
        if (collectionName.value() == value)
            return false;

        collectionName.setValue(value.toString());
    } break;
    case SourceRole: {
        auto sourceAddress = collectionSource.variantProperty(CollectionEditor::SOURCEFILE_PROPERTY);
        if (sourceAddress.value() == value)
            return false;

        sourceAddress.setValue(value.toString());
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

    AbstractView *view = m_collectionSources.at(row).view();
    if (!view)
        return false;

    count = rowMax - row;

    bool selectionUpdateNeeded = m_selectedIndex >= row && m_selectedIndex < rowMax;

    // It's better to remove the group of nodes here because of the performance issue for the list,
    // and update issue for the view
    beginRemoveRows({}, row, rowMax - 1);

    view->executeInTransaction(Q_FUNC_INFO, [row, count, this]() {
        for (ModelNode node : Utils::span<const ModelNode>(m_collectionSources).subspan(row, count)) {
            m_sourceIndexHash.remove(node.internalId());
            node.destroy();
        }
        m_collectionSources.remove(row, count);
        m_collectionList.remove(row, count);
    });

    int idx = row;
    for (const ModelNode &node : Utils::span<const ModelNode>(m_collectionSources).subspan(row))
        m_sourceIndexHash.insert(node.internalId(), ++idx);

    endRemoveRows();

    if (selectionUpdateNeeded)
        updateSelectedSource();

    updateEmpty();
    return true;
}

QHash<int, QByteArray> CollectionSourceModel::roleNames() const
{
    static QHash<int, QByteArray> roles;
    if (roles.isEmpty()) {
        roles.insert(Super::roleNames());
        roles.insert({{IdRole, "sourceId"},
                      {NameRole, "sourceName"},
                      {SelectedRole, "sourceIsSelected"},
                      {SourceRole, "sourceAddress"},
                      {CollectionsRole, "collections"}});
    }
    return roles;
}

void CollectionSourceModel::setSources(const ModelNodes &sources)
{
    beginResetModel();
    m_collectionSources = sources;
    m_sourceIndexHash.clear();
    m_collectionList.clear();
    int i = -1;
    for (const ModelNode &collectionSource : sources) {
        m_sourceIndexHash.insert(collectionSource.internalId(), ++i);

        auto loadedCollection = loadCollection(collectionSource);
        m_collectionList.append(loadedCollection);

        connect(loadedCollection.data(),
                &CollectionListModel::selectedIndexChanged,
                this,
                &CollectionSourceModel::onSelectedCollectionChanged,
                Qt::UniqueConnection);
    }

    updateEmpty();
    endResetModel();

    updateSelectedSource(true);
}

void CollectionSourceModel::removeSource(const ModelNode &node)
{
    int nodePlace = m_sourceIndexHash.value(node.internalId(), -1);
    if (nodePlace < 0)
        return;

    removeRow(nodePlace);
}

int CollectionSourceModel::sourceIndex(const ModelNode &node) const
{
    return m_sourceIndexHash.value(node.internalId(), -1);
}

void CollectionSourceModel::addSource(const ModelNode &node)
{
    int newRowId = m_collectionSources.count();
    beginInsertRows({}, newRowId, newRowId);
    m_collectionSources.append(node);
    m_sourceIndexHash.insert(node.internalId(), newRowId);

    auto loadedCollection = loadCollection(node);
    m_collectionList.append(loadedCollection);

    connect(loadedCollection.data(),
            &CollectionListModel::selectedIndexChanged,
            this,
            &CollectionSourceModel::onSelectedCollectionChanged,
            Qt::UniqueConnection);

    updateEmpty();
    endInsertRows();
    updateSelectedSource(true);
}

void CollectionSourceModel::selectSource(const ModelNode &node)
{
    int nodePlace = m_sourceIndexHash.value(node.internalId(), -1);
    if (nodePlace < 0)
        return;

    selectSourceIndex(nodePlace, true);
}

QmlDesigner::ModelNode CollectionSourceModel::sourceNodeAt(int idx)
{
    QModelIndex data = index(idx);
    if (!data.isValid())
        return {};

    return m_collectionSources.at(idx);
}

CollectionListModel *CollectionSourceModel::selectedCollectionList()
{
    QModelIndex idx = index(m_selectedIndex);
    if (!idx.isValid())
        return {};

    return idx.data(CollectionsRole).value<CollectionListModel *>();
}

void CollectionSourceModel::selectSourceIndex(int idx, bool selectAtLeastOne)
{
    int collectionCount = m_collectionSources.size();
    int preferredIndex = -1;
    if (collectionCount) {
        if (selectAtLeastOne)
            preferredIndex = std::max(0, std::min(idx, collectionCount - 1));
        else if (idx > -1 && idx < collectionCount)
            preferredIndex = idx;
    }

    setSelectedIndex(preferredIndex);
}

void CollectionSourceModel::deselect()
{
    setSelectedIndex(-1);
}

void CollectionSourceModel::updateSelectedSource(bool selectAtLeastOne)
{
    int idx = m_selectedIndex;
    m_selectedIndex = -1;
    selectSourceIndex(idx, selectAtLeastOne);
}

void CollectionSourceModel::updateNodeName(const ModelNode &node)
{
    QModelIndex index = indexOfNode(node);
    emit dataChanged(index, index, {NameRole, Qt::DisplayRole});
    updateCollectionList(index);
}

void CollectionSourceModel::updateNodeSource(const ModelNode &node)
{
    QModelIndex index = indexOfNode(node);
    emit dataChanged(index, index, {SourceRole});
    updateCollectionList(index);
}

void CollectionSourceModel::updateNodeId(const ModelNode &node)
{
    QModelIndex index = indexOfNode(node);
    emit dataChanged(index, index, {IdRole});
}

QString CollectionSourceModel::selectedSourceAddress() const
{
    return index(m_selectedIndex).data(SourceRole).toString();
}

void CollectionSourceModel::onSelectedCollectionChanged(int collectionIndex)
{
    CollectionListModel *collectionList = qobject_cast<CollectionListModel *>(sender());
    if (collectionIndex > -1 && collectionList) {
        if (_previousSelectedList && _previousSelectedList != collectionList)
            _previousSelectedList->selectCollectionIndex(-1);

        emit collectionSelected(collectionList->sourceNode(),
                                collectionList->collectionNameAt(collectionIndex));

        _previousSelectedList = collectionList;
    }
}

void CollectionSourceModel::setSelectedIndex(int idx)
{
    idx = (idx > -1 && idx < m_collectionSources.count()) ? idx : -1;

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
    bool isEmptyNow = m_collectionSources.isEmpty();
    if (m_isEmpty != isEmptyNow) {
        m_isEmpty = isEmptyNow;
        emit isEmptyChanged(m_isEmpty);

        if (m_isEmpty)
            setSelectedIndex(-1);
    }
}

void CollectionSourceModel::updateCollectionList(QModelIndex index)
{
    if (!index.isValid())
        return;

    ModelNode sourceNode = sourceNodeAt(index.row());
    QSharedPointer<CollectionListModel> currentList = m_collectionList.at(index.row());
    QSharedPointer<CollectionListModel> newList = loadCollection(sourceNode, currentList);
    if (currentList != newList) {
        m_collectionList.replace(index.row(), newList);
        emit this->dataChanged(index, index, {CollectionsRole});
    }
}

QModelIndex CollectionSourceModel::indexOfNode(const ModelNode &node) const
{
    return index(m_sourceIndexHash.value(node.internalId(), -1));
}
} // namespace QmlDesigner
