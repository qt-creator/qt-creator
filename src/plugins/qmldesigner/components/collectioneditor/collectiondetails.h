// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "modelnode.h"

#include <QSharedPointer>

QT_BEGIN_NAMESPACE
class QJsonObject;
struct QJsonParseError;
class QVariant;
QT_END_NAMESPACE

namespace QmlDesigner {

struct CollectionReference
{
    ModelNode node;
    QString name;

    friend auto qHash(const CollectionReference &collection)
    {
        return qHash(collection.node) ^ ::qHash(collection.name);
    }

    bool operator==(const CollectionReference &other) const
    {
        return node == other.node && name == other.name;
    }

    bool operator!=(const CollectionReference &other) const { return !(*this == other); }
};

struct CollectionProperty;

struct DataTypeWarning {
public:
    enum Warning { None, CellDataTypeMismatch };
    Q_ENUM(Warning)

    Warning warning = None;
    DataTypeWarning(Warning warning)
        : warning(warning)
    {}

    static QString getDataTypeWarningString(Warning warning)
    {
        return dataTypeWarnings.value(warning);
    }

private:
    Q_GADGET
    static const QMap<Warning, QString> dataTypeWarnings;
};

struct CollectionParseError
{
    enum ParseError {
        NoError,
        MainObjectMissing,
        CollectionNameNotFound,
        CollectionIsNotObject,
        ColumnsBlockIsNotArray,
        UnknownError
    };

    ParseError errorNo = ParseError::NoError;
    QString errorString() const;
};

class CollectionDetails
{
    Q_GADGET

public:
    enum class DataType { Unknown, String, Url, Integer, Real, Boolean, Image, Color };
    Q_ENUM(DataType)

    explicit CollectionDetails();
    CollectionDetails(const CollectionReference &reference);
    CollectionDetails(const CollectionDetails &other);
    ~CollectionDetails();

    void resetData(const QJsonDocument &localDocument,
                   const QString &collectionToImport,
                   CollectionParseError *error = nullptr);

    void insertColumn(const QString &propertyName,
                      int colIdx = -1,
                      const QVariant &defaultValue = {},
                      DataType type = DataType::String);
    bool removeColumns(int colIdx, int count = 1);

    void insertEmptyRows(int row = 0, int count = 1);
    bool removeRows(int row, int count = 1);
    bool setPropertyValue(int row, int column, const QVariant &value);

    bool setPropertyName(int column, const QString &value);
    bool setPropertyType(int column, DataType type);

    CollectionReference reference() const;
    QVariant data(int row, int column) const;
    QString propertyAt(int column) const;
    DataType typeAt(int column) const;
    DataType typeAt(int row, int column) const;
    DataTypeWarning::Warning cellWarningCheck(int row, int column) const;
    bool containsPropertyName(const QString &propertyName) const;

    bool hasValidReference() const;
    bool isChanged() const;

    int columns() const;
    int rows() const;

    bool markSaved();

    void swap(CollectionDetails &other);
    void resetReference(const CollectionReference &reference);

    QString toJson() const;
    QString toCsv() const;
    QJsonObject toLocalJson() const;

    static void registerDeclarativeType();

    static CollectionDetails fromImportedCsv(const QByteArray &document,
                                             const bool &firstRowIsHeader = true);
    static QList<CollectionDetails> fromImportedJson(const QByteArray &jsonContent,
                                                     QJsonParseError *error = nullptr);
    static CollectionDetails fromLocalJson(const QJsonDocument &document,
                                           const QString &collectionName,
                                           CollectionParseError *error = nullptr);

    CollectionDetails &operator=(const CollectionDetails &other);

private:
    void markChanged();
    void insertRecords(const QJsonArray &record, int idx = -1, int count = 1);

    static CollectionDetails fromImportedJson(const QJsonArray &importedArray,
                                              const QStringList &propertyPriority = {});
    static CollectionDetails fromLocalCollection(const QJsonObject &localCollection,
                                                 CollectionParseError *error = nullptr);

    // The private data is supposed to be shared between the copies
    class Private;
    QSharedPointer<Private> d;
};

} // namespace QmlDesigner
