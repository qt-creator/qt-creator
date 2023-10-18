// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "collectioneditorconstants.h"
#include "modelnode.h"

#include <QSharedPointer>

QT_BEGIN_NAMESPACE
class QJsonObject;
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

class CollectionDetails
{
    Q_GADGET

public:
    enum class DataType { Unknown, String, Url, Number, Boolean, Image, Color };
    Q_ENUM(DataType)

    explicit CollectionDetails();
    CollectionDetails(const CollectionReference &reference);
    CollectionDetails(const CollectionDetails &other);
    ~CollectionDetails();

    void resetDetails(const QStringList &propertyNames,
                      const QList<QJsonObject> &elements,
                      CollectionEditor::SourceFormat format);
    void insertColumn(const QString &propertyName,
                      int colIdx = -1,
                      const QVariant &defaultValue = {},
                      DataType type = DataType::Unknown);
    bool removeColumns(int colIdx, int count = 1);

    void insertElementAt(std::optional<QJsonObject> object, int row = -1);
    void insertEmptyElements(int row = 0, int count = 1);
    bool removeElements(int row, int count = 1);
    bool setPropertyValue(int row, int column, const QVariant &value);

    bool setPropertyName(int column, const QString &value);
    bool forcePropertyType(int column, DataType type, bool force = false);

    CollectionReference reference() const;
    CollectionEditor::SourceFormat sourceFormat() const;
    QVariant data(int row, int column) const;
    QString propertyAt(int column) const;
    DataType typeAt(int column) const;
    DataType typeAt(int row, int column) const;
    bool containsPropertyName(const QString &propertyName);

    bool isValid() const;
    bool isChanged() const;

    int columns() const;
    int rows() const;

    bool markSaved();

    void swap(CollectionDetails &other);
    QJsonArray getJsonCollection() const;
    QString getCsvCollection() const;

    static void registerDeclarativeType();

    CollectionDetails &operator=(const CollectionDetails &other);

private:
    void markChanged();
    void resetPropertyType(const QString &propertyName);
    void resetPropertyType(CollectionProperty &property);
    void resetPropertyTypes();

    // The private data is supposed to be shared between the copies
    class Private;
    QSharedPointer<Private> d;
};
} // namespace QmlDesigner
