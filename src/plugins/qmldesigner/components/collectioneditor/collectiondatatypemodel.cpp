// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "collectiondatatypemodel.h"

#include <QHash>
#include <QtQml/QmlTypeAndRevisionsRegistration>

namespace QmlDesigner {

struct CollectionDataTypeModel::Details
{
    CollectionDetails::DataType type;
    QString name;
    QString description;
};

const QList<CollectionDataTypeModel::Details> CollectionDataTypeModel::m_orderedDetails{
    {DataType::String, "String", "Text"},
    {DataType::Integer, "Integer", "Whole number that can be positive, negative, or zero"},
    {DataType::Real, "Real", "Number with a decimal"},
    {DataType::Image, "Image", "Image resource"},
    {DataType::Color, "Color", "HEX value"},
    {DataType::Url, "Url", "Resource locator"},
    {DataType::Boolean, "Boolean", "True/false"},
    {DataType::Unknown, "Unknown", "Unknown data type"},
};

CollectionDataTypeModel::CollectionDataTypeModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int CollectionDataTypeModel::rowCount([[maybe_unused]] const QModelIndex &parent) const
{
    return m_orderedDetails.size();
}

QVariant CollectionDataTypeModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return {};

    if (role == Qt::DisplayRole)
        return m_orderedDetails.at(index.row()).name;
    if (role == Qt::ToolTipRole)
        return m_orderedDetails.at(index.row()).description;

    return {};
}

QString CollectionDataTypeModel::dataTypeToString(DataType dataType)
{
    static const QHash<DataType, QString> dataTypeHash = []() -> QHash<DataType, QString> {
        QHash<DataType, QString> result;
        for (const Details &details : m_orderedDetails)
            result.insert(details.type, details.name);
        return result;
    }();

    if (dataTypeHash.contains(dataType))
        return dataTypeHash.value(dataType);

    return "Unknown";
}

CollectionDetails::DataType CollectionDataTypeModel::dataTypeFromString(const QString &dataType)
{
    static const QHash<QString, DataType> stringTypeHash = []() -> QHash<QString, DataType> {
        QHash<QString, DataType> result;
        for (const Details &details : m_orderedDetails)
            result.insert(details.name, details.type);
        return result;
    }();

    if (stringTypeHash.contains(dataType))
        return stringTypeHash.value(dataType);

    return DataType::Unknown;
}

void CollectionDataTypeModel::registerDeclarativeType()
{
    qmlRegisterType<CollectionDataTypeModel>("CollectionDetails", 1, 0, "CollectionDataTypeModel");
}

} // namespace QmlDesigner
