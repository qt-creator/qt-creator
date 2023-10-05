// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "collectiondetails.h"

#include <QJsonObject>
#include <QVariant>

namespace QmlDesigner {

class CollectionDetails::Private
{
    using SourceFormat = CollectionEditor::SourceFormat;

public:
    QStringList headers;
    QList<QJsonObject> elements;
    SourceFormat sourceFormat = SourceFormat::Unknown;
    CollectionReference reference;
    bool isChanged = false;

    bool isValidColumnId(int column) const { return column > -1 && column < headers.size(); }

    bool isValidRowId(int row) const { return row > -1 && row < elements.size(); }
};

CollectionDetails::CollectionDetails()
    : d(new Private())
{}

CollectionDetails::CollectionDetails(const CollectionReference &reference)
    : CollectionDetails()
{
    d->reference = reference;
}

CollectionDetails::CollectionDetails(const CollectionDetails &other) = default;

CollectionDetails::~CollectionDetails() = default;

void CollectionDetails::resetDetails(const QStringList &headers,
                                     const QList<QJsonObject> &elements,
                                     CollectionEditor::SourceFormat format)
{
    if (!isValid())
        return;

    d->headers = headers;
    d->elements = elements;
    d->sourceFormat = format;

    markSaved();
}

void CollectionDetails::insertHeader(const QString &header, int place, const QVariant &defaultValue)
{
    if (!isValid())
        return;

    if (d->headers.contains(header))
        return;

    if (d->isValidColumnId(place))
        d->headers.insert(place, header);
    else
        d->headers.append(header);

    QJsonValue defaultJsonValue = QJsonValue::fromVariant(defaultValue);
    for (QJsonObject &element : d->elements)
        element.insert(header, defaultJsonValue);

    markChanged();
}

void CollectionDetails::removeHeader(int place)
{
    if (!isValid())
        return;

    if (!d->isValidColumnId(place))
        return;

    const QString header = d->headers.takeAt(place);

    for (QJsonObject &element : d->elements)
        element.remove(header);

    markChanged();
}

void CollectionDetails::insertElementAt(std::optional<QJsonObject> object, int row)
{
    if (!isValid())
        return;

    auto insertJson = [this, row](const QJsonObject &jsonObject) {
        if (d->isValidRowId(row))
            d->elements.insert(row, jsonObject);
        else
            d->elements.append(jsonObject);
    };

    if (object.has_value()) {
        insertJson(object.value());
    } else {
        QJsonObject defaultObject;
        for (const QString &header : std::as_const(d->headers))
            defaultObject.insert(header, {});
        insertJson(defaultObject);
    }

    markChanged();
}

CollectionReference CollectionDetails::reference() const
{
    return d->reference;
}

CollectionEditor::SourceFormat CollectionDetails::sourceFormat() const
{
    return d->sourceFormat;
}

QVariant CollectionDetails::data(int row, int column) const
{
    if (!isValid())
        return {};

    if (!d->isValidRowId(row))
        return {};

    if (!d->isValidColumnId(column))
        return {};

    const QString &propertyName = d->headers.at(column);
    const QJsonObject &elementNode = d->elements.at(row);

    if (elementNode.contains(propertyName))
        return elementNode.value(propertyName).toVariant();

    return {};
}

QString CollectionDetails::headerAt(int column) const
{
    if (!d->isValidColumnId(column))
        return {};

    return d->headers.at(column);
}

bool CollectionDetails::isValid() const
{
    return d->reference.node.isValid() && d->reference.name.size();
}

bool CollectionDetails::isChanged() const
{
    return d->isChanged;
}

int CollectionDetails::columns() const
{
    return d->headers.size();
}

int CollectionDetails::rows() const
{
    return d->elements.size();
}

bool CollectionDetails::markSaved()
{
    if (d->isChanged) {
        d->isChanged = false;
        return true;
    }
    return false;
}

void CollectionDetails::swap(CollectionDetails &other)
{
    d.swap(other.d);
}

CollectionDetails &CollectionDetails::operator=(const CollectionDetails &other)
{
    CollectionDetails value(other);
    swap(value);
    return *this;
}

void CollectionDetails::markChanged()
{
    d->isChanged = true;
}

} // namespace QmlDesigner
