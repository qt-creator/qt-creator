// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "collectionsourcemodel.h"

#include "abstractview.h"
#include "collectioneditorconstants.h"
#include "collectioneditorutils.h"
#include "collectionlistmodel.h"
#include "variantproperty.h"

#include <utils/qtcassert.h>
#include <qqml.h>

#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>

namespace {

QSharedPointer<QmlDesigner::CollectionListModel> loadCollection(
    const QmlDesigner::ModelNode &sourceNode,
    QSharedPointer<QmlDesigner::CollectionListModel> initialCollection = {})
{
    using namespace QmlDesigner::CollectionEditor;
    QString sourceFileAddress = getSourceCollectionPath(sourceNode);

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

CollectionSourceModel::CollectionSourceModel(QObject *parent)
    : Super(parent)
{}

int CollectionSourceModel::rowCount(const QModelIndex &) const
{
    return m_collectionSources.size();
}

QVariant CollectionSourceModel::data(const QModelIndex &index, int role) const
{
    QTC_ASSERT(index.isValid(), return {});

    const ModelNode *collectionSource = &m_collectionSources.at(index.row());

    switch (role) {
    case NameRole: // Not used, to be removed
        return collectionSource->variantProperty("objectName").value().toString();
    case NodeRole:
        return QVariant::fromValue(*collectionSource);
    case CollectionTypeRole:
        return CollectionEditor::getSourceCollectionType(*collectionSource);
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
        roles.insert({{NameRole, "sourceName"},
                      {NodeRole, "sourceNode"},
                      {CollectionTypeRole, "sourceCollectionType"},
                      {SelectedRole, "sourceIsSelected"},
                      {SourceRole, "sourceAddress"},
                      {CollectionsRole, "internalModels"}});
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

        registerCollection(loadedCollection);
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

    registerCollection(loadedCollection);

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

bool CollectionSourceModel::collectionExists(const ModelNode &node, const QString &collectionName) const
{
    int idx = sourceIndex(node);
    if (idx < 0)
        return false;

    auto collections = m_collectionList.at(idx);
    if (collections.isNull())
        return false;

    return collections->contains(collectionName);
}

bool CollectionSourceModel::addCollectionToSource(const ModelNode &node,
                                                  const QString &collectionName,
                                                  const QJsonArray &newCollectionData,
                                                  QString *errorString)
{
    auto returnError = [errorString](const QString &msg) -> bool {
        if (errorString)
            *errorString = msg;
        return false;
    };

    int idx = sourceIndex(node);
    if (idx < 0)
        return returnError(tr("Node is not indexed in the models."));

    if (node.type() != CollectionEditor::JSONCOLLECTIONMODEL_TYPENAME)
        return returnError(tr("Node should be a JSON model."));

    if (collectionExists(node, collectionName))
        return returnError(tr("A model with the identical name already exists."));

    QString sourceFileAddress = CollectionEditor::getSourceCollectionPath(node);

    QFileInfo sourceFileInfo(sourceFileAddress);
    if (!sourceFileInfo.isFile())
        return returnError(tr("Selected node must have a valid source file address"));

    QFile jsonFile(sourceFileAddress);
    if (!jsonFile.open(QFile::ReadWrite))
        return returnError(tr("Can't read or write \"%1\".\n%2")
                               .arg(sourceFileInfo.absoluteFilePath(), jsonFile.errorString()));

    QJsonParseError parseError;
    QJsonDocument document = QJsonDocument::fromJson(jsonFile.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError)
        return returnError(tr("\"%1\" is corrupted.\n%2")
                               .arg(sourceFileInfo.absoluteFilePath(), parseError.errorString()));

    if (document.isObject()) {
        QJsonObject sourceObject = document.object();
        sourceObject.insert(collectionName, newCollectionData);
        document.setObject(sourceObject);
        if (!jsonFile.resize(0))
            return returnError(tr("Can't clean \"%1\".").arg(sourceFileInfo.absoluteFilePath()));

        QByteArray jsonData = document.toJson();
        auto writtenBytes = jsonFile.write(jsonData);
        jsonFile.close();

        if (writtenBytes != jsonData.size())
            return returnError(tr("Can't write to \"%1\".").arg(sourceFileInfo.absoluteFilePath()));

        updateCollectionList(index(idx));

        auto collections = m_collectionList.at(idx);
        if (collections.isNull())
            return returnError(tr("No model is available for the JSON model group."));

        collections->selectCollectionName(collectionName);
        return true;
    } else {
        return returnError(tr("JSON document type should be an object containing models."));
    }
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

bool CollectionSourceModel::collectionExists(const QVariant &node, const QString &collectionName) const
{
    return collectionExists(node.value<ModelNode>(), collectionName);
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

void CollectionSourceModel::onSelectedCollectionChanged(CollectionListModel *collectionList,
                                                        int collectionIndex)
{
    if (collectionIndex > -1) {
        if (m_previousSelectedList && m_previousSelectedList != collectionList)
            m_previousSelectedList->selectCollectionIndex(-1);

        m_previousSelectedList = collectionList;

        emit collectionSelected(collectionList->sourceNode(),
                                collectionList->collectionNameAt(collectionIndex));

        selectSourceIndex(sourceIndex(collectionList->sourceNode()));
    }
}

void CollectionSourceModel::onCollectionNameChanged(CollectionListModel *collectionList,
                                                    const QString &oldName, const QString &newName)
{
    auto emitRenameWarning = [this](const QString &msg) -> void {
        emit this->warning(tr("Rename Model"), msg);
    };

    const ModelNode node = collectionList->sourceNode();
    const QModelIndex nodeIndex = indexOfNode(node);

    if (!nodeIndex.isValid()) {
        emitRenameWarning(tr("Invalid node"));
        return;
    }

    if (node.type() == CollectionEditor::CSVCOLLECTIONMODEL_TYPENAME) {
        if (!setData(nodeIndex, newName, NameRole))
            emitRenameWarning(tr("Can't rename the node"));
        return;
    } else if (node.type() != CollectionEditor::JSONCOLLECTIONMODEL_TYPENAME) {
        emitRenameWarning(tr("Invalid node type"));
        return;
    }

    QString sourceFileAddress = CollectionEditor::getSourceCollectionPath(node);

    QFileInfo sourceFileInfo(sourceFileAddress);
    if (!sourceFileInfo.isFile()) {
        emitRenameWarning(tr("Selected node must have a valid source file address"));
        return;
    }

    QFile jsonFile(sourceFileAddress);
    if (!jsonFile.open(QFile::ReadWrite)) {
        emitRenameWarning(tr("Can't read or write \"%1\".\n%2")
                              .arg(sourceFileInfo.absoluteFilePath(), jsonFile.errorString()));
        return;
    }

    QJsonParseError parseError;
    QJsonDocument document = QJsonDocument::fromJson(jsonFile.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        emitRenameWarning(tr("\"%1\" is corrupted.\n%2")
                              .arg(sourceFileInfo.absoluteFilePath(), parseError.errorString()));
        return;
    }

    if (document.isObject()) {
        QJsonObject rootObject = document.object();

        bool collectionContainsOldName = rootObject.contains(oldName);
        bool collectionContainsNewName = rootObject.contains(newName);

        if (!collectionContainsOldName) {
            emitRenameWarning(
                tr("The model group doesn't contain the old model name (%1).").arg(oldName));
            return;
        }

        if (collectionContainsNewName) {
            emitRenameWarning(
                tr("The model name \"%1\" already exists in the model group.").arg(newName));
            return;
        }

        QJsonValue oldValue = rootObject.value(oldName);
        rootObject.insert(newName, oldValue);
        rootObject.remove(oldName);

        document.setObject(rootObject);
        if (!jsonFile.resize(0)) {
            emitRenameWarning(tr("Can't clean \"%1\".").arg(sourceFileInfo.absoluteFilePath()));
            return;
        }

        QByteArray jsonData = document.toJson();
        auto writtenBytes = jsonFile.write(jsonData);
        jsonFile.close();

        if (writtenBytes != jsonData.size()) {
            emitRenameWarning(tr("Can't write to \"%1\".").arg(sourceFileInfo.absoluteFilePath()));
            return;
        }

        updateCollectionList(nodeIndex);
    }
}

void CollectionSourceModel::onCollectionsRemoved(CollectionListModel *collectionList,
                                                 const QStringList &removedCollections)
{
    auto emitDeleteWarning = [this](const QString &msg) -> void {
        emit warning(tr("Delete Model"), msg);
    };

    const ModelNode node = collectionList->sourceNode();
    const QModelIndex nodeIndex = indexOfNode(node);

    if (!nodeIndex.isValid()) {
        emitDeleteWarning(tr("Invalid node"));
        return;
    }

    if (node.type() == CollectionEditor::CSVCOLLECTIONMODEL_TYPENAME) {
        removeSource(node);
        return;
    } else if (node.type() != CollectionEditor::JSONCOLLECTIONMODEL_TYPENAME) {
        emitDeleteWarning(tr("Invalid node type"));
        return;
    }

    QString sourceFileAddress = CollectionEditor::getSourceCollectionPath(node);

    QFileInfo sourceFileInfo(sourceFileAddress);
    if (!sourceFileInfo.isFile()) {
        emitDeleteWarning(tr("The selected node has an invalid source address"));
        return;
    }

    QFile jsonFile(sourceFileAddress);
    if (!jsonFile.open(QFile::ReadWrite)) {
        emitDeleteWarning(tr("Can't read or write \"%1\".\n%2")
                              .arg(sourceFileInfo.absoluteFilePath(), jsonFile.errorString()));
        return;
    }

    QJsonParseError parseError;
    QJsonDocument document = QJsonDocument::fromJson(jsonFile.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        emitDeleteWarning(tr("\"%1\" is corrupted.\n%2")
                              .arg(sourceFileInfo.absoluteFilePath(), parseError.errorString()));
        return;
    }

    if (document.isObject()) {
        QJsonObject rootObject = document.object();

        for (const QString &collectionName : removedCollections) {
            bool sourceContainsCollection = rootObject.contains(collectionName);
            if (sourceContainsCollection) {
                rootObject.remove(collectionName);
            } else {
                emitDeleteWarning(tr("The model group doesn't contain the model name (%1).")
                                      .arg(sourceContainsCollection));
            }
        }

        document.setObject(rootObject);
        if (!jsonFile.resize(0)) {
            emitDeleteWarning(tr("Can't clean \"%1\".").arg(sourceFileInfo.absoluteFilePath()));
            return;
        }

        QByteArray jsonData = document.toJson();
        auto writtenBytes = jsonFile.write(jsonData);
        jsonFile.close();

        if (writtenBytes != jsonData.size()) {
            emitDeleteWarning(tr("Can't write to \"%1\".").arg(sourceFileInfo.absoluteFilePath()));
            return;
        }

        updateCollectionList(nodeIndex);
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

        if (idx > -1) {
            QPointer<CollectionListModel> relatedCollectionList = m_collectionList.at(idx).data();
            if (relatedCollectionList) {
                if (relatedCollectionList->selectedIndex() < 0)
                    relatedCollectionList->selectCollectionIndex(0, true);
            } else if (m_previousSelectedList) {
                m_previousSelectedList->selectCollectionIndex(-1);
                m_previousSelectedList = {};
                emit this->collectionSelected(sourceNodeAt(idx), {});
            }
        }
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
        emit dataChanged(index, index, {CollectionsRole});
        emit collectionNamesChanged(sourceNode, newList->stringList());
    }
}

void CollectionSourceModel::registerCollection(const QSharedPointer<CollectionListModel> &collection)
{
    CollectionListModel *collectionList = collection.data();
    if (collectionList == nullptr)
        return;

    connect(collectionList, &CollectionListModel::selectedIndexChanged, this,
        [this, collectionList](int idx) {
            onSelectedCollectionChanged(collectionList, idx);
        }, Qt::UniqueConnection);

    connect(collectionList, &CollectionListModel::collectionNameChanged, this,
        [this, collectionList](const QString &oldName, const QString &newName) {
            onCollectionNameChanged(collectionList, oldName, newName);
        }, Qt::UniqueConnection);

    connect(collectionList, &CollectionListModel::collectionsRemoved, this,
        [this, collectionList](const QStringList &removedCollections) {
            onCollectionsRemoved(collectionList, removedCollections);
        }, Qt::UniqueConnection);

    if (collection->sourceNode())
        emit collectionNamesChanged(collection->sourceNode(), collection->stringList());
}

QModelIndex CollectionSourceModel::indexOfNode(const ModelNode &node) const
{
    return index(m_sourceIndexHash.value(node.internalId(), -1));
}

void CollectionJsonSourceFilterModel::registerDeclarativeType()
{
    qmlRegisterType<CollectionJsonSourceFilterModel>("CollectionEditor",
                                                     1,
                                                     0,
                                                     "CollectionJsonSourceFilterModel");
}

bool CollectionJsonSourceFilterModel::filterAcceptsRow(int source_row, const QModelIndex &) const
{
    if (!sourceModel())
        return false;
    QModelIndex sourceItem = sourceModel()->index(source_row, 0, {});
    return sourceItem.data(CollectionSourceModel::Roles::CollectionTypeRole).toString() == "json";
}

} // namespace QmlDesigner
