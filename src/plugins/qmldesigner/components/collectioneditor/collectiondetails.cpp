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

inline static bool isValidColorName(const QString &colorName)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 4, 0)
    return QColor::isValidColorName(colorName);
#else
    constexpr QStringView colorPattern(
        u"(?<color>^(?:#(?:(?:[0-9a-fA-F]{2}){3,4}|(?:[0-9a-fA-F]){3,4}))$)");
    static const QRegularExpression colorRegex(colorPattern.toString());
    return colorRegex.match(colorName).hasMatch();
#endif // >= Qt 6.4
}

/**
 * @brief getCustomUrl
 * MimeType = <MainType/SubType>
 * Address = <Url|LocalFile>
 *
 * @param value The input value to be evaluated
 * @param dataType if the value is a valid url or image, the data type
 * will be stored to this parameter, otherwise, it will be Unknown
 * @param urlResult if the value is a valid url or image, the address
 * will be stored in this parameter, otherwise it will be empty.
 * @param subType if the value is a valid image, the image subtype
 * will be stored in this parameter, otherwise it will be empty.
 * @return true if the result is either url or image
 */
inline static bool getCustomUrl(const QString &value,
                                CollectionDetails::DataType &dataType,
                                QUrl *urlResult = nullptr,
                                QString *subType = nullptr)
{
    constexpr QStringView urlPattern(
        u"^(?<MimeType>"
        u"(?<MainType>image)\\/"
        u"(?<SubType>apng|avif|gif|jpeg|png|(?:svg\\+xml)|webp|xyz)\\:)?" // end of MimeType
        u"(?<Address>"
        u"(?<Url>https?:\\/\\/"
        u"(?:www\\.|(?!www))[A-z0-9][A-z0-9-]+[A-z0-9]\\.[^\\s]{2,}|www\\.[A-z0-9][A-z0-9-]+"
        u"[A-z0-9]\\.[^\\s]{2,}|https?:\\/\\/"
        u"(?:www\\.|(?!www))[A-z0-9]+\\.[^\\s]{2,}|www\\.[A-z0-9]+\\.[^\\s]{2,})|" // end of Url
        u"(?<LocalFile>("
        u"?:(?:[A-z]:)|(?:(?:\\\\|\\/){1,2}\\w+)\\$?)(?:(?:\\\\|\\/)(?:\\w[\\w ]*.*))+)" // end of LocalFile
        u"){1}$"); // end of Address
    static const QRegularExpression urlRegex(urlPattern.toString());

    const QRegularExpressionMatch match = urlRegex.match(value);
    if (match.hasMatch()) {
        if (match.hasCaptured("Address")) {
            if (match.hasCaptured("MimeType") && match.captured("MainType") == "image")
                dataType = CollectionDetails::DataType::Image;
            else
                dataType = CollectionDetails::DataType::Url;

            if (urlResult)
                urlResult->setUrl(match.captured("Address"));

            if (subType)
                *subType = match.captured("SubType");

            return true;
        }
    }

    if (urlResult)
        urlResult->clear();

    if (subType)
        subType->clear();

    dataType = CollectionDetails::DataType::Unknown;
    return false;
}

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
    case JsonType::Double: {
        if (qFuzzyIsNull(std::remainder(value.toDouble(), 1)))
            return DataType::Integer;
        return DataType::Real;
    }
    case JsonType::String: {
        const QString stringValue = value.toString();
        if (isValidColorName(stringValue))
            return DataType::Color;

        DataType urlType;
        if (getCustomUrl(stringValue, urlType))
            return urlType;

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
    case DataType::Integer:
        return variantValue.toInt();
    case DataType::Real:
        return variantValue.toDouble();
    case DataType::Boolean:
        return variantValue.toBool();
    case DataType::Color:
        return variantValue.value<QColor>();
    case DataType::Image: {
        DataType type;
        QUrl url;
        if (getCustomUrl(variantValue.toString(), type, &url))
            return url;
        return variantValue.toString();
    }
    case DataType::Url:
        return variantValue.value<QUrl>();
    default:
        return variantValue;
    }
}

static QJsonValue variantToJsonValue(
    const QVariant &variant, CollectionDetails::DataType type = CollectionDetails::DataType::Unknown)
{
    using VariantType = QVariant::Type;
    using DataType = CollectionDetails::DataType;

    if (type == CollectionDetails::DataType::Unknown) {
        static const QHash<VariantType, DataType> typeMap = {{VariantType::Bool, DataType::Boolean},
                                                             {VariantType::Double, DataType::Real},
                                                             {VariantType::Int, DataType::Integer},
                                                             {VariantType::String, DataType::String},
                                                             {VariantType::Color, DataType::Color},
                                                             {VariantType::Url, DataType::Url}};
        type = typeMap.value(variant.type(), DataType::Unknown);
    }

    switch (type) {
    case DataType::Boolean:
        return variant.toBool();
    case DataType::Real:
        return variant.toDouble();
    case DataType::Integer:
        return variant.toInt();
    case DataType::Image: {
        const QUrl url(variant.toUrl());
        if (url.isValid())
            return QString("image/xyz:%1").arg(url.toString());
        return {};
    }
    case DataType::String:
    case DataType::Color:
    case DataType::Url:
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

    element.insert(d->properties.at(column).name, variantToJsonValue(value, typeAt(column)));
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

    const DataType formerType = property.type;
    property.type = type;

    for (QJsonObject &element : d->elements) {
        if (element.contains(property.name)) {
            const QJsonValue value = element.value(property.name);
            const QVariant properTypedValue = valueToVariant(value, formerType);
            const QJsonValue properTypedJsonValue = variantToJsonValue(properTypedValue, type);
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

    if (!elementNode.contains(propertyName))
        return {};

    const QJsonValue cellValue = elementNode.value(propertyName);

    if (typeAt(column) == DataType::Image) {
        const QUrl imageUrl = valueToVariant(cellValue, DataType::Image).toUrl();

        if (imageUrl.isValid())
            return imageUrl;
    }

    return cellValue.toVariant();
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

    const DataType columnType = typeAt(column);
    const DataType cellType = typeAt(row, column);
    if (columnType == DataType::Unknown || element.isEmpty()
        || data(row, column) == QVariant::fromValue(nullptr)) {
        return DataTypeWarning::Warning::None;
    }

    if (element.contains(propertyName)) {
        if (columnType == DataType::Real && cellType == DataType::Integer)
            return DataTypeWarning::Warning::None;
        else if (columnType != cellType)
            return DataTypeWarning::Warning::CellDataTypeMismatch;
    }

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
    DataType columnType = DataType::Unknown;
    for (const QJsonObject &element : std::as_const(d->elements)) {
        if (element.contains(propertyName)) {
            const DataType cellType = collectionDataTypeFromJsonValue(element.value(propertyName));
            if (cellType != DataType::Unknown) {
                if (columnType == DataType::Integer && cellType != DataType::Real)
                    continue;

                columnType = cellType;
                if (columnType == DataType::Integer)
                    continue;

                break;
            }
        }
    }
    property.type = columnType;
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
