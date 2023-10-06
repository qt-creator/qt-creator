// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "singlecollectionmodel.h"

#include "collectioneditorconstants.h"
#include "modelnode.h"
#include "variantproperty.h"

#include <utils/qtcassert.h>

#include <QFile>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonParseError>

namespace {

QStringList getJsonHeaders(const QJsonArray &collectionArray)
{
    QSet<QString> result;
    for (const QJsonValue &value : collectionArray) {
        if (value.isObject()) {
            const QJsonObject object = value.toObject();
            const QStringList headers = object.toVariantMap().keys();
            for (const QString &header : headers)
                result.insert(header);
        }
    }

    return result.values();
}
} // namespace

namespace QmlDesigner {

SingleCollectionModel::SingleCollectionModel(QObject *parent)
    : QAbstractTableModel(parent)
{}

QHash<int, QByteArray> SingleCollectionModel::roleNames() const
{
    static QHash<int, QByteArray> roles;
    if (roles.isEmpty()) {
        roles.insert(QAbstractTableModel::roleNames());
        roles.insert(SelectedRole, "itemSelected");
    }
    return roles;
}

int SingleCollectionModel::rowCount([[maybe_unused]] const QModelIndex &parent) const
{
    return m_currentCollection.rows();
}

int SingleCollectionModel::columnCount([[maybe_unused]] const QModelIndex &parent) const
{
    return m_currentCollection.columns();
}

QVariant SingleCollectionModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return {};

    if (role == SelectedRole)
        return index.column() == m_selectedColumn;

    return m_currentCollection.data(index.row(), index.column());
}

bool SingleCollectionModel::setData(const QModelIndex &, const QVariant &, int)
{
    return false;
}

bool SingleCollectionModel::setHeaderData(int section,
                                          Qt::Orientation orientation,
                                          const QVariant &value,
                                          [[maybe_unused]] int role)
{
    if (orientation == Qt::Vertical)
        return false;

    bool headerChanged = m_currentCollection.setHeader(section, value.toString());
    if (headerChanged)
        emit this->headerDataChanged(orientation, section, section);

    return headerChanged;
}

bool SingleCollectionModel::removeColumns(int column, int count, const QModelIndex &parent)
{
    if (column < 0 || column >= columnCount(parent) || count < 1)
        return false;

    count = std::min(count, columnCount(parent) - column);
    beginRemoveColumns(parent, column, column + count);
    bool columnsRemoved = m_currentCollection.removeColumns(column, count);
    endRemoveColumns();

    return columnsRemoved;
}

Qt::ItemFlags SingleCollectionModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return {};

    return {Qt::ItemIsSelectable | Qt::ItemIsEnabled};
}

QVariant SingleCollectionModel::headerData(int section,
                                           Qt::Orientation orientation,
                                           [[maybe_unused]] int role) const
{
    if (orientation == Qt::Horizontal)
        return m_currentCollection.headerAt(section);

    return {};
}

int SingleCollectionModel::selectedColumn() const
{
    return m_selectedColumn;
}

bool SingleCollectionModel::selectColumn(int section)
{
    if (m_selectedColumn == section)
        return false;

    const int columns = columnCount();

    if (m_selectedColumn >= columns)
        return false;

    const int rows = rowCount();
    const int previousColumn = m_selectedColumn;

    m_selectedColumn = section;
    emit this->selectedColumnChanged(m_selectedColumn);

    auto notifySelectedDataChanged = [this, columns, rows](int notifyingColumn) {
        if (notifyingColumn > -1 && notifyingColumn < columns && rows) {
            emit dataChanged(index(0, notifyingColumn),
                             index(rows - 1, notifyingColumn),
                             {SelectedRole});
        }
    };

    notifySelectedDataChanged(previousColumn);
    notifySelectedDataChanged(m_selectedColumn);

    return true;
}

bool SingleCollectionModel::renameColumn(int section, const QString &newValue)
{
    return setHeaderData(section, Qt::Horizontal, newValue);
}

void SingleCollectionModel::loadCollection(const ModelNode &sourceNode, const QString &collection)
{
    QString fileName = sourceNode.variantProperty(CollectionEditor::SOURCEFILE_PROPERTY).value().toString();

    CollectionReference newReference{sourceNode, collection};
    bool alreadyOpen = m_openedCollections.contains(newReference);

    if (alreadyOpen) {
        if (m_currentCollection.reference() != newReference) {
            selectColumn(-1);
            beginResetModel();
            switchToCollection(newReference);
            endResetModel();
        }
    } else {
        selectColumn(-1);
        switchToCollection(newReference);
        if (sourceNode.type() == CollectionEditor::JSONCOLLECTIONMODEL_TYPENAME)
            loadJsonCollection(fileName, collection);
        else if (sourceNode.type() == CollectionEditor::CSVCOLLECTIONMODEL_TYPENAME)
            loadCsvCollection(fileName, collection);
    }
}

void SingleCollectionModel::switchToCollection(const CollectionReference &collection)
{
    if (m_currentCollection.reference() == collection)
        return;

    closeCurrentCollectionIfSaved();

    if (!m_openedCollections.contains(collection))
        m_openedCollections.insert(collection, CollectionDetails(collection));

    m_currentCollection = m_openedCollections.value(collection);

    setCollectionName(collection.name);
}

void SingleCollectionModel::closeCollectionIfSaved(const CollectionReference &collection)
{
    if (!m_openedCollections.contains(collection))
        return;

    const CollectionDetails &collectionDetails = m_openedCollections.value(collection);

    if (!collectionDetails.isChanged())
        m_openedCollections.remove(collection);

    m_currentCollection = CollectionDetails{};
}

void SingleCollectionModel::closeCurrentCollectionIfSaved()
{
    if (m_currentCollection.isValid())
        closeCollectionIfSaved(m_currentCollection.reference());
}

void SingleCollectionModel::loadJsonCollection(const QString &source, const QString &collection)
{
    using CollectionEditor::SourceFormat;

    QFile sourceFile(source);
    QJsonArray collectionNodes;
    bool jsonFileIsOk = false;
    if (sourceFile.open(QFile::ReadOnly)) {
        QJsonParseError jpe;
        QJsonDocument document = QJsonDocument::fromJson(sourceFile.readAll(), &jpe);
        if (jpe.error == QJsonParseError::NoError) {
            jsonFileIsOk = true;
            if (document.isObject()) {
                QJsonObject collectionMap = document.object();
                if (collectionMap.contains(collection)) {
                    QJsonValue collectionVal = collectionMap.value(collection);
                    if (collectionVal.isArray())
                        collectionNodes = collectionVal.toArray();
                    else
                        collectionNodes.append(collectionVal);
                }
            }
        }
    }

    if (collectionNodes.isEmpty()) {
        closeCurrentCollectionIfSaved();
        endResetModel();
        return;
    };

    QList<QJsonObject> elements;
    for (const QJsonValue &value : std::as_const(collectionNodes)) {
        if (value.isObject()) {
            QJsonObject object = value.toObject();
            elements.append(object);
        }
    }

    SourceFormat sourceFormat = jsonFileIsOk ? SourceFormat::Json : SourceFormat::Unknown;

    beginResetModel();
    m_currentCollection.resetDetails(getJsonHeaders(collectionNodes), elements, sourceFormat);
    endResetModel();
}

void SingleCollectionModel::loadCsvCollection(const QString &source,
                                              [[maybe_unused]] const QString &collectionName)
{
    using CollectionEditor::SourceFormat;

    QFile sourceFile(source);
    QStringList headers;
    QList<QJsonObject> elements;
    bool csvFileIsOk = false;

    if (sourceFile.open(QFile::ReadOnly)) {
        QTextStream stream(&sourceFile);

        if (!stream.atEnd())
            headers = stream.readLine().split(',');

        if (!headers.isEmpty()) {
            while (!stream.atEnd()) {
                const QStringList recordDataList = stream.readLine().split(',');
                int column = -1;
                QJsonObject recordData;
                for (const QString &cellData : recordDataList) {
                    if (++column == headers.size())
                        break;
                    recordData.insert(headers.at(column), cellData);
                }
                if (recordData.count())
                    elements.append(recordData);
            }
            csvFileIsOk = true;
        }
    }

    SourceFormat sourceFormat = csvFileIsOk ? SourceFormat::Csv : SourceFormat::Unknown;

    beginResetModel();
    m_currentCollection.resetDetails(headers, elements, sourceFormat);
    endResetModel();
}

void SingleCollectionModel::setCollectionName(const QString &newCollectionName)
{
    if (m_collectionName != newCollectionName) {
        m_collectionName = newCollectionName;
        emit this->collectionNameChanged(m_collectionName);
    }
}

bool SingleCollectionModel::saveCollectionAsJson(const QString &collection, const QJsonArray &content, const QString &source)
{
    QFile sourceFile(source);
    if (sourceFile.open(QFile::ReadWrite)) {
        QJsonParseError jpe;
        QJsonDocument document = QJsonDocument::fromJson(sourceFile.readAll(), &jpe);

        if (jpe.error == QJsonParseError::NoError) {
            QJsonObject collectionMap = document.object();
            collectionMap[collection] = content;
            document.setObject(collectionMap);
        }

        sourceFile.resize(0);
        if (sourceFile.write(document.toJson()))
            return true;
    }

    return false;
}

} // namespace QmlDesigner
