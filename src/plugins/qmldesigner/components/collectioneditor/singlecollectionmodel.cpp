// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "singlecollectionmodel.h"

#include "collectioneditorconstants.h"
#include "modelnode.h"
#include "variantproperty.h"

#include <utils/qtcassert.h>

#include <QFile>
#include <QJsonArray>
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

int SingleCollectionModel::rowCount([[maybe_unused]] const QModelIndex &parent) const
{
    return m_elements.count();
}

int SingleCollectionModel::columnCount([[maybe_unused]] const QModelIndex &parent) const
{
    return m_headers.count();
}

QVariant SingleCollectionModel::data(const QModelIndex &index, int) const
{
    if (!index.isValid())
        return {};

    const QString &propertyName = m_headers.at(index.column());
    const QJsonObject &elementNode = m_elements.at(index.row());

    if (elementNode.contains(propertyName))
        return elementNode.value(propertyName).toVariant();

    return {};
}

bool SingleCollectionModel::setData(const QModelIndex &, const QVariant &, int)
{
    return false;
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
        return m_headers.at(section);

    return {};
}

void SingleCollectionModel::loadCollection(const ModelNode &sourceNode, const QString &collection)
{
    QString fileName = sourceNode.variantProperty(CollectionEditor::SOURCEFILE_PROPERTY).value().toString();

    if (sourceNode.type() == CollectionEditor::JSONCOLLECTIONMODEL_TYPENAME)
        loadJsonCollection(fileName, collection);
    else if (sourceNode.type() == CollectionEditor::CSVCOLLECTIONMODEL_TYPENAME)
        loadCsvCollection(fileName, collection);
}

void SingleCollectionModel::loadJsonCollection(const QString &source, const QString &collection)
{
    beginResetModel();
    setCollectionName(collection);
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

    setCollectionSourceFormat(jsonFileIsOk ? SourceFormat::Json : SourceFormat::Unknown);

    if (collectionNodes.isEmpty()) {
        m_headers.clear();
        m_elements.clear();
        endResetModel();
        return;
    }

    m_headers = getJsonHeaders(collectionNodes);

    m_elements.clear();
    for (const QJsonValue &value : std::as_const(collectionNodes)) {
        if (value.isObject()) {
            QJsonObject object = value.toObject();
            m_elements.append(object);
        }
    }

    endResetModel();
}

void SingleCollectionModel::loadCsvCollection(const QString &source, const QString &collectionName)
{
    beginResetModel();

    setCollectionName(collectionName);
    QFile sourceFile(source);
    m_headers.clear();
    m_elements.clear();
    bool csvFileIsOk = false;

    if (sourceFile.open(QFile::ReadOnly)) {
        QTextStream stream(&sourceFile);

        if (!stream.atEnd())
            m_headers = stream.readLine().split(',');

        if (!m_headers.isEmpty()) {
            while (!stream.atEnd()) {
                const QStringList recordDataList = stream.readLine().split(',');
                int column = -1;
                QJsonObject recordData;
                for (const QString &cellData : recordDataList) {
                    if (++column == m_headers.size())
                        break;
                    recordData.insert(m_headers.at(column), cellData);
                }
                if (recordData.count())
                    m_elements.append(recordData);
            }
            csvFileIsOk = true;
        }
    }

    setCollectionSourceFormat(csvFileIsOk ? SourceFormat::Csv : SourceFormat::Unknown);
    endResetModel();
}

void SingleCollectionModel::setCollectionName(const QString &newCollectionName)
{
    if (m_collectionName != newCollectionName) {
        m_collectionName = newCollectionName;
        emit this->collectionNameChanged(m_collectionName);
    }
}

void SingleCollectionModel::setCollectionSourceFormat(SourceFormat sourceFormat)
{
    m_sourceFormat = sourceFormat;
}
} // namespace QmlDesigner
