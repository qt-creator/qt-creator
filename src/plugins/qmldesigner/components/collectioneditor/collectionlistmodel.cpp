// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "collectionlistmodel.h"

#include "collectioneditorutils.h"

#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/qtcassert.h>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>

namespace {

template<typename ValueType>
bool containsItem(const std::initializer_list<ValueType> &container, const ValueType &value)
{
    auto begin = std::cbegin(container);
    auto end = std::cend(container);

    auto it = std::find(begin, end, value);
    return it != end;
}

bool sameCollectionNames(QStringList a, QStringList b)
{
    if (a.size() != b.size())
        return false;

    a.sort(Qt::CaseSensitive);
    b.sort(Qt::CaseSensitive);

    return a == b;
}

} // namespace

namespace QmlDesigner {

CollectionListModel::CollectionListModel()
    : QAbstractListModel()
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
        if (collectionExists(value.toString()))
            return false;

        QString oldName = collectionNameAt(index.row());
        bool nameChanged = value != data(index);
        if (nameChanged) {
            QString newName = value.toString();
            QString errorString;
            if (renameCollectionInDataStore(oldName, newName, errorString)) {
                m_data.replace(index.row(), newName);
                emit dataChanged(index, index, {Qt::DisplayRole, Qt::EditRole, NameRole});
                emit this->collectionNameChanged(oldName, newName);
                if (m_selectedCollectionName == oldName)
                    updateSelectedCollectionName();
                return true;
            } else {
                emit warning("Rename Model", errorString);
                return false;
            }
        }
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

    QString errorString;
    QStringList removedCollections = m_data.mid(row, count);
    if (removeCollectionsFromDataStore(removedCollections, errorString)) {
        beginRemoveRows(parent, row, row + count - 1);
        m_data.remove(row, count);
        endRemoveRows();

        emit collectionsRemoved(removedCollections);
        if (m_selectedIndex >= row) {
            int preferredIndex = m_selectedIndex - count;
            if (preferredIndex < 0) // If the selected item is deleted, reset selection
                selectCollectionIndex(-1);
            selectCollectionIndex(preferredIndex, true);
        }

        updateSelectedCollectionName();
        return true;
    } else {
        emit warning("Remove Model", errorString);
        return false;
    }
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

void CollectionListModel::setDataStoreNode(const ModelNode &dataStoreNode)
{
    m_dataStoreNode = dataStoreNode;
    update();
}

int CollectionListModel::selectedIndex() const
{
    return m_selectedIndex;
}

ModelNode CollectionListModel::sourceNode() const
{
    return m_dataStoreNode;
}

bool CollectionListModel::collectionExists(const QString &collectionName) const
{
    return m_data.contains(collectionName);
}

QStringList CollectionListModel::collections() const
{
    return m_data;
}

QString CollectionListModel::getUniqueCollectionName(const QString &baseName) const
{
    QString name = baseName.isEmpty() ? "Model" : baseName;
    QString nameTemplate = name + "%1";

    int num = 0;

    while (collectionExists(name))
        name = nameTemplate.arg(++num, 2, 10, QChar('0'));

    return name;
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

void CollectionListModel::selectCollectionName(QString collectionName, bool selectAtLeastOne)
{
    int idx = m_data.indexOf(collectionName);
    if (idx > -1)
        selectCollectionIndex(idx);
    else
        selectCollectionIndex(selectedIndex(), selectAtLeastOne);

    collectionName = collectionNameAt(selectedIndex());
    if (m_selectedCollectionName == collectionName)
        return;

    m_selectedCollectionName = collectionName;
    emit selectedCollectionNameChanged(m_selectedCollectionName);
}

QString CollectionListModel::collectionNameAt(int idx) const
{
    return index(idx).data(NameRole).toString();
}

QString CollectionListModel::selectedCollectionName() const
{
    return m_selectedCollectionName;
}

void CollectionListModel::update()
{
    using Utils::FilePath;
    using Utils::FileReader;

    FileReader sourceFile;
    QString sourceFileAddress = CollectionEditorUtils::getSourceCollectionPath(m_dataStoreNode);
    FilePath path = FilePath::fromUserInput(sourceFileAddress);
    bool fileRead = false;
    if (path.exists()) {
        fileRead = sourceFile.fetch(path);
        if (!fileRead)
            emit this->warning(tr("Model Editor"),
                               tr("Cannot read the dataStore file\n%1").arg(sourceFile.errorString()));
    }

    QStringList collectionNames;
    if (fileRead) {
        QJsonParseError parseError;
        QJsonDocument document = QJsonDocument::fromJson(sourceFile.data(), &parseError);
        if (parseError.error != QJsonParseError::NoError) {
            emit this->warning(tr("Model Editor"),
                               tr("There is an error in the JSON file.\n%1")
                                   .arg(parseError.errorString()));
        } else {
            if (document.isObject())
                collectionNames = document.object().toVariantMap().keys();
            else
                emit this->warning(tr("Model Editor"), tr("The JSON document be an object."));
        }
    }

    if (!sameCollectionNames(m_data, collectionNames)) {
        QString prevSelectedCollection = selectedIndex() > -1 ? m_data.at(selectedIndex())
                                                              : QString();
        beginResetModel();
        m_data = collectionNames;
        endResetModel();
        emit this->collectionNamesChanged(collections());
        selectCollectionName(prevSelectedCollection, true);
    }
}

bool CollectionListModel::addCollection(const QString &collectionName,
                                        const QJsonObject &localCollection)
{
    if (collectionExists(collectionName)) {
        emit warning(tr("Add Model"), tr("Model \"%1\" already exists.").arg(collectionName));
        return false;
    }

    QString errorMessage;
    if (addCollectionToDataStore(collectionName, localCollection, errorMessage)) {
        int row = rowCount();
        beginInsertRows({}, row, row);
        m_data.append(collectionName);
        endInsertRows();

        selectCollectionName(collectionName);
        emit collectionAdded(collectionName);
        return true;
    } else {
        emit warning(tr("Add Collection"), errorMessage);
    }
    return false;
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
        updateSelectedCollectionName();
    }
}

bool CollectionListModel::removeCollectionsFromDataStore(const QStringList &removedCollections,
                                                         QString &error) const
{
    using Utils::FilePath;
    using Utils::FileReader;
    auto setErrorAndReturn = [&error](const QString &msg) -> bool {
        error = msg;
        return false;
    };

    if (m_dataStoreNode.type() != CollectionEditorConstants::JSONCOLLECTIONMODEL_TYPENAME)
        return setErrorAndReturn(tr("Invalid node type"));

    QString sourceFileAddress = CollectionEditorUtils::getSourceCollectionPath(m_dataStoreNode);

    QFileInfo sourceFileInfo(sourceFileAddress);
    if (!sourceFileInfo.isFile())
        return setErrorAndReturn(tr("The selected node has an invalid source address"));

    FilePath jsonPath = FilePath::fromUserInput(sourceFileAddress);
    FileReader jsonFile;
    if (!jsonFile.fetch(jsonPath)) {
        return setErrorAndReturn(tr("Can't read file \"%1\".\n%2")
                                     .arg(sourceFileInfo.absoluteFilePath(), jsonFile.errorString()));
    }

    QJsonParseError parseError;
    QJsonDocument document = QJsonDocument::fromJson(jsonFile.data(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        return setErrorAndReturn(tr("\"%1\" is corrupted.\n%2")
                                     .arg(sourceFileInfo.absoluteFilePath(), parseError.errorString()));
    }

    if (document.isObject()) {
        QJsonObject rootObject = document.object();

        for (const QString &collectionName : removedCollections) {
            bool sourceContainsCollection = rootObject.contains(collectionName);
            if (sourceContainsCollection) {
                rootObject.remove(collectionName);
            } else {
                setErrorAndReturn(tr("The model group doesn't contain the model name (%1).")
                                      .arg(sourceContainsCollection));
            }
        }

        document.setObject(rootObject);

        if (CollectionEditorUtils::writeToJsonDocument(jsonPath, document)) {
            error.clear();
            return true;
        } else {
            return setErrorAndReturn(
                tr("Can't write to \"%1\".").arg(sourceFileInfo.absoluteFilePath()));
        }
    } else {
        return setErrorAndReturn(tr("Local Json Document should be an object"));
    }

    return false;
}

bool CollectionListModel::renameCollectionInDataStore(const QString &oldName,
                                                      const QString &newName,
                                                      QString &error)
{
    using Utils::FilePath;
    using Utils::FileReader;
    using Utils::FileSaver;

    auto setErrorAndReturn = [&error](const QString &msg) -> bool {
        error = msg;
        return false;
    };

    if (m_dataStoreNode.type() != CollectionEditorConstants::JSONCOLLECTIONMODEL_TYPENAME)
        return setErrorAndReturn(tr("Invalid node type"));

    QString sourceFileAddress = CollectionEditorUtils::getSourceCollectionPath(m_dataStoreNode);

    QFileInfo sourceFileInfo(sourceFileAddress);
    if (!sourceFileInfo.isFile())
        return setErrorAndReturn(tr("Selected node must have a valid source file address"));

    FilePath jsonPath = FilePath::fromUserInput(sourceFileAddress);
    FileReader jsonFile;
    if (!jsonFile.fetch(jsonPath)) {
        return setErrorAndReturn(
            tr("Can't read \"%1\".\n%2").arg(sourceFileInfo.absoluteFilePath(), jsonFile.errorString()));
    }

    QJsonParseError parseError;
    QJsonDocument document = QJsonDocument::fromJson(jsonFile.data(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        return setErrorAndReturn(tr("\"%1\" is corrupted.\n%2")
                                     .arg(sourceFileInfo.absoluteFilePath(), parseError.errorString()));
    }

    if (document.isObject()) {
        QJsonObject rootObject = document.object();

        bool collectionContainsOldName = rootObject.contains(oldName);
        bool collectionContainsNewName = rootObject.contains(newName);

        if (!collectionContainsOldName) {
            return setErrorAndReturn(
                tr("The model group doesn't contain the old model name (%1).").arg(oldName));
        }

        if (collectionContainsNewName) {
            return setErrorAndReturn(
                tr("The model name \"%1\" already exists in the model group.").arg(newName));
        }

        QJsonValue oldValue = rootObject.value(oldName);
        rootObject.insert(newName, oldValue);
        rootObject.remove(oldName);

        document.setObject(rootObject);

        if (CollectionEditorUtils::writeToJsonDocument(jsonPath, document)) {
            error.clear();
            return true;
        } else {
            return setErrorAndReturn(
                tr("Can't write to \"%1\".").arg(sourceFileInfo.absoluteFilePath()));
        }
    } else {
        return setErrorAndReturn(tr("Local Json Document should be an object"));
    }
    return false;
}

bool CollectionListModel::addCollectionToDataStore(const QString &collectionName,
                                                   const QJsonObject &localCollection,
                                                   QString &errorString) const
{
    using Utils::FilePath;
    using Utils::FileReader;
    auto returnError = [&errorString](const QString &msg) -> bool {
        errorString = msg;
        return false;
    };

    if (collectionExists(collectionName))
        return returnError(tr("A model with the identical name already exists."));

    QString sourceFileAddress = CollectionEditorUtils::getSourceCollectionPath(m_dataStoreNode);

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
        sourceObject.insert(collectionName, localCollection);
        document.setObject(sourceObject);

        if (CollectionEditorUtils::writeToJsonDocument(jsonPath, document))
            return true;
        else
            return returnError(tr("Can't write to \"%1\".").arg(sourceFileInfo.absoluteFilePath()));
    } else {
        return returnError(tr("JSON document type should be an object containing models."));
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

void CollectionListModel::updateSelectedCollectionName()
{
    QString selectedCollectionByIndex = collectionNameAt(selectedIndex());
    if (selectedCollectionByIndex != selectedCollectionName())
        selectCollectionName(selectedCollectionByIndex);
}

} // namespace QmlDesigner
