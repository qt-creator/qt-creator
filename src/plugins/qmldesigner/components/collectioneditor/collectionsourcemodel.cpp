// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "collectionsourcemodel.h"

#include "abstractview.h"
#include "collectioneditorconstants.h"
#include "collectioneditorutils.h"
#include "collectionlistmodel.h"
#include "variantproperty.h"

#include <utils/fileutils.h>
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
    using namespace QmlDesigner::CollectionEditorConstants;
    using namespace QmlDesigner::CollectionEditorUtils;
    using Utils::FilePath;
    using Utils::FileReader;
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
        FileReader sourceFile;
        if (!sourceFile.fetch(FilePath::fromUserInput(sourceFileAddress)))
            return {};

        QJsonParseError parseError;
        QJsonDocument document = QJsonDocument::fromJson(sourceFile.data(), &parseError);
        if (parseError.error != QJsonParseError::NoError)
            return {};

        setupCollectionList();

        if (document.isObject()) {
            const QJsonObject sourceObject = document.object();
            collectionsList->resetModelData(sourceObject.toVariantMap().keys());
        }
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
        return CollectionEditorUtils::getSourceCollectionType(*collectionSource);
    case SourceRole:
        return collectionSource->variantProperty(CollectionEditorConstants::SOURCEFILE_PROPERTY).value();
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
        auto sourceAddress = collectionSource.variantProperty(
            CollectionEditorConstants::SOURCEFILE_PROPERTY);
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

void CollectionSourceModel::setSource(const ModelNode &source)
{
    beginResetModel();
    m_collectionSources = {source};
    m_sourceIndexHash.clear();
    m_collectionList.clear();

    // TODO: change m_collectionSources to only contain 1 source node
    m_sourceIndexHash.insert(source.internalId(), 0);

    auto loadedCollection = loadCollection(source);
    m_collectionList.append(loadedCollection);

    registerCollectionList(loadedCollection);


    updateEmpty();
    endResetModel();

    updateSelectedSource(true);
}

void CollectionSourceModel::reset()
{
    beginResetModel();
    m_collectionSources.clear();
    m_sourceIndexHash.clear();
    m_collectionList.clear();
    m_previousSelectedList.clear();
    setSelectedCollectionName({});

    updateEmpty();
    endResetModel();
    updateSelectedSource();
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

    registerCollectionList(loadedCollection);

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

bool CollectionSourceModel::collectionExists(const QString &collectionName) const
{
    return  m_collectionList.size() == 1 && m_collectionList.at(0)->contains(collectionName);
}

bool CollectionSourceModel::addCollectionToSource(const ModelNode &node,
                                                  const QString &collectionName,
                                                  const QJsonObject &newCollection,
                                                  QString *errorString)
{
    using Utils::FilePath;
    using Utils::FileReader;
    auto returnError = [errorString](const QString &msg) -> bool {
        if (errorString)
            *errorString = msg;
        return false;
    };

    int idx = sourceIndex(node);
    if (idx < 0)
        return returnError(tr("Node is not indexed in the models."));

    if (node.type() != CollectionEditorConstants::JSONCOLLECTIONMODEL_TYPENAME)
        return returnError(tr("Node should be a JSON model."));

    if (collectionExists(collectionName))
        return returnError(tr("A model with the identical name already exists."));

    QString sourceFileAddress = CollectionEditorUtils::getSourceCollectionPath(node);

    QFileInfo sourceFileInfo(sourceFileAddress);
    if (!sourceFileInfo.isFile())
        return returnError(tr("Selected node must have a valid source file address"));

    FilePath jsonPath = FilePath::fromUserInput(sourceFileAddress);
    FileReader jsonFile;
    if (!jsonFile.fetch(jsonPath)) {
        return returnError(
            tr("Can't read \"%1\".\n%2").arg(sourceFileInfo.absoluteFilePath(), jsonFile.errorString()));
    }

    QJsonParseError parseError;
    QJsonDocument document = QJsonDocument::fromJson(jsonFile.data(), &parseError);
    if (parseError.error != QJsonParseError::NoError)
        return returnError(tr("\"%1\" is corrupted.\n%2")
                               .arg(sourceFileInfo.absoluteFilePath(), parseError.errorString()));

    if (document.isObject()) {
        QJsonObject sourceObject = document.object();
        sourceObject.insert(collectionName, newCollection);
        document.setObject(sourceObject);

        if (!CollectionEditorUtils::writeToJsonDocument(jsonPath, document))
            return returnError(tr("Can't write to \"%1\".").arg(sourceFileInfo.absoluteFilePath()));

        updateCollectionList(index(idx));

        auto collections = m_collectionList.at(idx);
        if (collections.isNull())
            return returnError(tr("No model is available for the JSON model group."));

        collections->selectCollectionName(collectionName);
        setSelectedCollectionName(collectionName);
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

void CollectionSourceModel::selectCollection(const QVariant &node, const QString &collectionName)
{
    const ModelNode sourceNode = node.value<ModelNode>();
    const QModelIndex index = indexOfNode(sourceNode);
    if (!index.isValid())
        return;

    selectSource(sourceNode);
    auto collections = m_collectionList.at(index.row());
    if (collections.isNull())
        return;

    collections->selectCollectionName(collectionName);
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

QString CollectionSourceModel::getUniqueCollectionName(const QString &baseName) const
{
    if (m_collectionList.isEmpty())
        return "Model01";

    CollectionListModel *collectionModel = m_collectionList.at(0).data();

    QString name = baseName.isEmpty() ? "Model" : baseName;
    QString nameTemplate = name + "%1";

    int num = 0;

    while (collectionModel->contains(name))
        name = nameTemplate.arg(++num, 2, 10, QChar('0'));

    return name;
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

        setSelectedCollectionName(collectionList->collectionNameAt(collectionIndex));

        selectSourceIndex(sourceIndex(collectionList->sourceNode()));
    } else {
        setSelectedCollectionName({});
    }
}

void CollectionSourceModel::onCollectionNameChanged(CollectionListModel *collectionList,
                                                    const QString &oldName,
                                                    const QString &newName)
{
    using Utils::FilePath;
    using Utils::FileReader;
    using Utils::FileSaver;

    auto emitRenameWarning = [this](const QString &msg) -> void {
        emit warning(tr("Rename Model"), msg);
    };

    const ModelNode node = collectionList->sourceNode();
    const QModelIndex nodeIndex = indexOfNode(node);

    if (!nodeIndex.isValid()) {
        emitRenameWarning(tr("Invalid node"));
        return;
    }

    if (node.type() != CollectionEditorConstants::JSONCOLLECTIONMODEL_TYPENAME) {
        emitRenameWarning(tr("Invalid node type"));
        return;
    }

    QString sourceFileAddress = CollectionEditorUtils::getSourceCollectionPath(node);

    QFileInfo sourceFileInfo(sourceFileAddress);
    if (!sourceFileInfo.isFile()) {
        emitRenameWarning(tr("Selected node must have a valid source file address"));
        return;
    }

    FilePath jsonPath = FilePath::fromUserInput(sourceFileAddress);
    FileReader jsonFile;
    if (!jsonFile.fetch(jsonPath)) {
        emitRenameWarning(
            tr("Can't read \"%1\".\n%2").arg(sourceFileInfo.absoluteFilePath(), jsonFile.errorString()));
        return;
    }

    QJsonParseError parseError;
    QJsonDocument document = QJsonDocument::fromJson(jsonFile.data(), &parseError);
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

        if (!CollectionEditorUtils::writeToJsonDocument(jsonPath, document)) {
            emitRenameWarning(tr("Can't write to \"%1\".").arg(sourceFileInfo.absoluteFilePath()));
            return;
        }

        CollectionListModel *list = m_collectionList.at(nodeIndex.row()).data();
        bool updateSelectedNames = list && list == m_previousSelectedList.data();
        emit collectionRenamed(oldName, newName);
        updateCollectionList(nodeIndex);

        if (updateSelectedNames) {
            list = m_collectionList.at(nodeIndex.row()).data();
            if (m_selectedCollectionName == oldName) {
                list->selectCollectionName(newName);
                setSelectedCollectionName(newName);
            } else {
                // reselect to update ID if it's changed due to renaming and order changes
                list->selectCollectionName(m_selectedCollectionName);
            }
        }
    }
}

void CollectionSourceModel::onCollectionsRemoved(CollectionListModel *collectionList,
                                                 const QStringList &removedCollections)
{
    using Utils::FilePath;
    using Utils::FileReader;
    auto emitDeleteWarning = [this](const QString &msg) -> void {
        emit warning(tr("Delete Model"), msg);
    };

    const ModelNode node = collectionList->sourceNode();
    const QModelIndex nodeIndex = indexOfNode(node);

    if (!nodeIndex.isValid()) {
        emitDeleteWarning(tr("Invalid node"));
        return;
    }

    if (node.type() != CollectionEditorConstants::JSONCOLLECTIONMODEL_TYPENAME) {
        emitDeleteWarning(tr("Invalid node type"));
        return;
    }

    QString sourceFileAddress = CollectionEditorUtils::getSourceCollectionPath(node);

    QFileInfo sourceFileInfo(sourceFileAddress);
    if (!sourceFileInfo.isFile()) {
        emitDeleteWarning(tr("The selected node has an invalid source address"));
        return;
    }

    FilePath jsonPath = FilePath::fromUserInput(sourceFileAddress);
    FileReader jsonFile;
    if (!jsonFile.fetch(jsonPath)) {
        emitDeleteWarning(tr("Can't read or write \"%1\".\n%2")
                              .arg(sourceFileInfo.absoluteFilePath(), jsonFile.errorString()));
        return;
    }

    QJsonParseError parseError;
    QJsonDocument document = QJsonDocument::fromJson(jsonFile.data(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        emitDeleteWarning(tr("\"%1\" is corrupted.\n%2")
                              .arg(sourceFileInfo.absoluteFilePath(), parseError.errorString()));
        return;
    }

    if (document.isObject()) {
        QJsonObject rootObject = document.object();

        QStringList collectionsRemovedFromDocument;
        for (const QString &collectionName : removedCollections) {
            bool sourceContainsCollection = rootObject.contains(collectionName);
            if (sourceContainsCollection) {
                rootObject.remove(collectionName);
                collectionsRemovedFromDocument << collectionName;
            } else {
                emitDeleteWarning(tr("The model group doesn't contain the model name (%1).")
                                      .arg(sourceContainsCollection));
            }
        }

        document.setObject(rootObject);

        if (!CollectionEditorUtils::writeToJsonDocument(jsonPath, document)) {
            emitDeleteWarning(tr("Can't write to \"%1\".").arg(sourceFileInfo.absoluteFilePath()));
            return;
        }

        for (const QString &collectionName : std::as_const(collectionsRemovedFromDocument))
            emit collectionRemoved(collectionName);

        updateCollectionList(nodeIndex);
        if (m_previousSelectedList == collectionList)
            onSelectedCollectionChanged(collectionList, collectionList->selectedIndex());
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
                setSelectedCollectionName({});
            }
        }
    }
}

void CollectionSourceModel::setSelectedCollectionName(const QString &collectionName)
{
    if (m_selectedCollectionName != collectionName) {
        m_selectedCollectionName = collectionName;
        emit collectionSelected(m_selectedCollectionName);
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
    QSharedPointer<CollectionListModel> oldList = m_collectionList.at(index.row());
    QSharedPointer<CollectionListModel> newList = loadCollection(sourceNode, oldList);
    if (oldList != newList) {
        m_collectionList.replace(index.row(), newList);
        emit dataChanged(index, index, {CollectionsRole});
        registerCollectionList(newList);
    }
}

void CollectionSourceModel::registerCollectionList(
    const QSharedPointer<CollectionListModel> &sharedCollectionList)
{
    CollectionListModel *collectionList = sharedCollectionList.data();
    if (collectionList == nullptr)
        return;

    if (!collectionList->property("_is_registered_in_sourceModel").toBool()) {
        collectionList->setProperty("_is_registered_in_sourceModel", true);

        connect(collectionList,
                &CollectionListModel::selectedIndexChanged,
                this,
                [this, collectionList](int idx) { onSelectedCollectionChanged(collectionList, idx); });

        connect(collectionList,
                &CollectionListModel::collectionNameChanged,
                this,
                [this, collectionList](const QString &oldName, const QString &newName) {
                    onCollectionNameChanged(collectionList, oldName, newName);
                });

        connect(collectionList,
                &CollectionListModel::collectionsRemoved,
                this,
                [this, collectionList](const QStringList &removedCollections) {
                    onCollectionsRemoved(collectionList, removedCollections);
                });

        connect(collectionList, &CollectionListModel::modelReset, this, [this, collectionList]() {
            emit collectionNamesInitialized(collectionList->collections());
        });
    }

    if (collectionList->sourceNode().isValid())
        emit collectionNamesInitialized(collectionList->collections());
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
