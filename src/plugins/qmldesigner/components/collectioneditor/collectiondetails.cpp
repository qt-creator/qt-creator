// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "collectiondetails.h"

#include <utils/span.h>
#include <qqml.h>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>
#include <QVariant>

namespace QmlDesigner {

struct CollectionProperty
{
    using DataType = CollectionDetails::DataType;

    QString name;
    DataType type;
};

const QMap<DataTypeWarning::Warning, QString> DataTypeWarning::dataTypeWarnings = {
    {DataTypeWarning::CellDataTypeMismatch, "Cell and column data types do not match."}
};

class CollectionDetails::Private
{
    using SourceFormat = CollectionEditorConstants::SourceFormat;

public:
    QList<CollectionProperty> properties;
    QList<QJsonObject> elements;
    SourceFormat sourceFormat = SourceFormat::Unknown;
    CollectionReference reference;
    bool isChanged = false;

    bool isValidColumnId(int column) const { return column > -1 && column < properties.size(); }

    bool isValidRowId(int row) const { return row > -1 && row < elements.size(); }
};

static CollectionProperty::DataType collectionDataTypeFromJsonValue(const QJsonValue &value)
{
    using DataType = CollectionDetails::DataType;
    using JsonType = QJsonValue::Type;

    switch (value.type()) {
    case JsonType::Null:
    case JsonType::Undefined:
        return DataType::Unknown;
    case JsonType::Bool:
        return DataType::Boolean;
    case JsonType::Double:
        return DataType::Number;
    case JsonType::String: {
        // TODO: Image, Color, Url
        return DataType::String;
    } break;
    default:
        return DataType::Unknown;
    }
}

static QVariant valueToVariant(const QJsonValue &value, CollectionDetails::DataType type)
{
    using DataType = CollectionDetails::DataType;
    QVariant variantValue = value.toVariant();

    switch (type) {
    case DataType::String:
        return variantValue.toString();
    case DataType::Number:
        return variantValue.toDouble();
    case DataType::Boolean:
        return variantValue.toBool();
    case DataType::Color:
        return variantValue.value<QColor>();
    case DataType::Url:
        return variantValue.value<QUrl>();
    default:
        return variantValue;
    }
}

static QJsonValue variantToJsonValue(const QVariant &variant)
{
    using VariantType = QVariant::Type;

    switch (variant.type()) {
    case VariantType::Bool:
        return variant.toBool();
    case VariantType::Double:
    case VariantType::Int:
        return variant.toDouble();
    case VariantType::String:
    case VariantType::Color:
    case VariantType::Url:
    default:
        return variant.toString();
    }
}

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

void CollectionDetails::resetDetails(const QStringList &propertyNames,
                                     const QList<QJsonObject> &elements,
                                     CollectionEditorConstants::SourceFormat format)
{
    if (!isValid())
        return;

    d->properties = Utils::transform(propertyNames, [](const QString &name) -> CollectionProperty {
        return {name, DataType::Unknown};
    });

    d->elements = elements;
    d->sourceFormat = format;

    resetPropertyTypes();
    markSaved();
}

void CollectionDetails::insertColumn(const QString &propertyName,
                                     int colIdx,
                                     const QVariant &defaultValue,
                                     DataType type)
{
    if (!isValid())
        return;

    if (containsPropertyName(propertyName))
        return;

    CollectionProperty property = {propertyName, type};
    if (d->isValidColumnId(colIdx))
        d->properties.insert(colIdx, property);
    else
        d->properties.append(property);

    QJsonValue defaultJsonValue = QJsonValue::fromVariant(defaultValue);
    for (QJsonObject &element : d->elements)
        element.insert(propertyName, defaultJsonValue);

    markChanged();
}

bool CollectionDetails::removeColumns(int colIdx, int count)
{
    if (count < 1 || !isValid() || !d->isValidColumnId(colIdx))
        return false;

    int maxCount = d->properties.count() - colIdx;
    count = std::min(maxCount, count);

    const QList<CollectionProperty> removedProperties = d->properties.mid(colIdx, count);
    d->properties.remove(colIdx, count);

    for (const CollectionProperty &property : removedProperties) {
        for (QJsonObject &element : d->elements)
            element.remove(property.name);
    }

    markChanged();

    return true;
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
        for (const CollectionProperty &property : std::as_const(d->properties))
            defaultObject.insert(property.name, {});
        insertJson(defaultObject);
    }

    markChanged();
}

void CollectionDetails::insertEmptyElements(int row, int count)
{
    if (!isValid())
        return;

    if (count < 1)
        return;

    row = qBound(0, row, rows());
    d->elements.insert(row, count, {});

    markChanged();
}

bool CollectionDetails::removeElements(int row, int count)
{
    if (count < 1 || !isValid() || !d->isValidRowId(row))
        return false;

    int maxCount = d->elements.count() - row;
    count = std::min(maxCount, count);

    QSet<QString> removedProperties;
    Utils::span elementsSpan{std::as_const(d->elements)};
    for (const QJsonObject &element : elementsSpan.subspan(row, count)) {
        const QStringList elementPropertyNames = element.keys();
        for (const QString &removedProperty : elementPropertyNames)
            removedProperties.insert(removedProperty);
    }

    d->elements.remove(row, count);

    for (const QString &removedProperty : removedProperties)
        resetPropertyType(removedProperty);

    markChanged();

    return true;
}

bool CollectionDetails::setPropertyValue(int row, int column, const QVariant &value)
{
    if (!d->isValidRowId(row) || !d->isValidColumnId(column))
        return false;

    QJsonObject &element = d->elements[row];
    QVariant currentValue = data(row, column);

    if (value == currentValue)
        return false;

    element.insert(d->properties.at(column).name, QJsonValue::fromVariant(value));
    markChanged();
    return true;
}

bool CollectionDetails::setPropertyName(int column, const QString &value)
{
    if (!d->isValidColumnId(column))
        return false;

    const CollectionProperty &oldProperty = d->properties.at(column);
    const QString oldColumnName = oldProperty.name;
    if (oldColumnName == value)
        return false;

    d->properties.replace(column, {value, oldProperty.type});
    for (QJsonObject &element : d->elements) {
        if (element.contains(oldColumnName)) {
            element.insert(value, element.value(oldColumnName));
            element.remove(oldColumnName);
        }
    }

    markChanged();
    return true;
}

bool CollectionDetails::setPropertyType(int column, DataType type)
{
    if (!isValid() || !d->isValidColumnId(column))
        return false;

    bool changed = false;
    CollectionProperty &property = d->properties[column];
    if (property.type != type)
        changed = true;

    property.type = type;

    for (QJsonObject &element : d->elements) {
        if (element.contains(property.name)) {
            const QJsonValue value = element.value(property.name);
            const QVariant properTypedValue = valueToVariant(value, type);
            const QJsonValue properTypedJsonValue = variantToJsonValue(properTypedValue);
            element.insert(property.name, properTypedJsonValue);
            changed = true;
        }
    }

    if (changed)
        markChanged();

    return changed;
}

CollectionReference CollectionDetails::reference() const
{
    return d->reference;
}

CollectionEditorConstants::SourceFormat CollectionDetails::sourceFormat() const
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

    const QString &propertyName = d->properties.at(column).name;
    const QJsonObject &elementNode = d->elements.at(row);

    if (elementNode.contains(propertyName))
        return elementNode.value(propertyName).toVariant();

    return {};
}

QString CollectionDetails::propertyAt(int column) const
{
    if (!d->isValidColumnId(column))
        return {};

    return d->properties.at(column).name;
}

CollectionDetails::DataType CollectionDetails::typeAt(int column) const
{
    if (!d->isValidColumnId(column))
        return {};

    return d->properties.at(column).type;
}

CollectionDetails::DataType CollectionDetails::typeAt(int row, int column) const
{
    if (!d->isValidRowId(row) || !d->isValidColumnId(column))
        return {};

    const QString &propertyName = d->properties.at(column).name;
    const QJsonObject &element = d->elements.at(row);

    if (element.contains(propertyName))
        return collectionDataTypeFromJsonValue(element.value(propertyName));

    return {};
}

DataTypeWarning::Warning CollectionDetails::cellWarningCheck(int row, int column) const
{
    const QString &propertyName = d->properties.at(column).name;
    const QJsonObject &element = d->elements.at(row);

    if (typeAt(column) == DataType::Unknown || element.isEmpty()
        || data(row, column) == QVariant::fromValue(nullptr)) {
        return DataTypeWarning::Warning::None;
    }

    if (element.contains(propertyName) && typeAt(column) != typeAt(row, column))
        return DataTypeWarning::Warning::CellDataTypeMismatch;

    return DataTypeWarning::Warning::None;
}

bool CollectionDetails::containsPropertyName(const QString &propertyName)
{
    if (!isValid())
        return false;

    return Utils::anyOf(d->properties, [&propertyName](const CollectionProperty &property) {
        return property.name == propertyName;
    });
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
    return d->properties.size();
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

void CollectionDetails::registerDeclarativeType()
{
    typedef CollectionDetails::DataType DataType;
    qRegisterMetaType<DataType>("DataType");
    qmlRegisterUncreatableType<CollectionDetails>("CollectionDetails", 1, 0, "DataType", "Enum type");

    qRegisterMetaType<DataTypeWarning::Warning>("Warning");
    qmlRegisterUncreatableType<DataTypeWarning>("CollectionDetails", 1, 0, "Warning", "Enum type");
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

void CollectionDetails::resetPropertyType(const QString &propertyName)
{
    for (CollectionProperty &property : d->properties) {
        if (property.name == propertyName)
            resetPropertyType(property);
    }
}

void CollectionDetails::resetPropertyType(CollectionProperty &property)
{
    const QString &propertyName = property.name;
    DataType type = DataType::Unknown;
    for (const QJsonObject &element : std::as_const(d->elements)) {
        if (element.contains(propertyName)) {
            type = collectionDataTypeFromJsonValue(element.value(propertyName));
            if (type != DataType::Unknown)
                break;
        }
    }

    property.type = type;
}

void CollectionDetails::resetPropertyTypes()
{
    for (CollectionProperty &property : d->properties)
        resetPropertyType(property);
}

QJsonArray CollectionDetails::getCollectionAsJsonArray() const
{
    QJsonArray collectionArray;

    for (const QJsonObject &element : std::as_const(d->elements))
        collectionArray.push_back(element);

    return collectionArray;
}

QString CollectionDetails::getCollectionAsJsonString() const
{
    return QString::fromUtf8(QJsonDocument(getCollectionAsJsonArray()).toJson());
}

QString CollectionDetails::getCollectionAsCsvString() const
{
    QString content;
    if (d->properties.count() <= 0)
        return "";

    for (const CollectionProperty &property : std::as_const(d->properties))
        content += property.name + ',';

    content.back() = '\n';

    for (const QJsonObject &elementsRow : std::as_const(d->elements)) {
        for (const CollectionProperty &property : std::as_const(d->properties)) {
            const QJsonValue &value = elementsRow.value(property.name);

            if (value.isDouble())
                content += QString::number(value.toDouble()) + ',';
            else
                content += value.toString() + ',';
        }
        content.back() = '\n';
    }

    return content;
}

} // namespace QmlDesigner
