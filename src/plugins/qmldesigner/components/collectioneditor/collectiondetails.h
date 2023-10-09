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

class CollectionDetails
{
public:
    explicit CollectionDetails();
    CollectionDetails(const CollectionReference &reference);
    CollectionDetails(const CollectionDetails &other);
    ~CollectionDetails();

    void resetDetails(const QStringList &headers,
                      const QList<QJsonObject> &elements,
                      CollectionEditor::SourceFormat format);
    void insertColumn(const QString &header, int colIdx = -1, const QVariant &defaultValue = {});
    bool removeColumns(int colIdx, int count = 1);

    void insertElementAt(std::optional<QJsonObject> object, int row = -1);
    void insertEmptyElements(int row = 0, int count = 1);

    bool setHeader(int column, const QString &value);

    CollectionReference reference() const;
    CollectionEditor::SourceFormat sourceFormat() const;
    QVariant data(int row, int column) const;
    QString headerAt(int column) const;
    bool containsHeader(const QString &header);

    bool isValid() const;
    bool isChanged() const;

    int columns() const;
    int rows() const;

    bool markSaved();

    void swap(CollectionDetails &other);
    QJsonArray getJsonCollection() const;
    CollectionDetails &operator=(const CollectionDetails &other);

private:
    void markChanged();

    // The private data is supposed to be shared between the copies
    class Private;
    QSharedPointer<Private> d;
};

} // namespace QmlDesigner
