// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "collectiondetails.h"

#include "collectiondatatypemodel.h"
#include "collectioneditorutils.h"

#include <utils/span.h>
#include <qmljs/parser/qmljsast_p.h>
#include <qmljs/parser/qmljsastvisitor_p.h>
#include <qmljs/qmljsdocument.h>
#include <qqml.h>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QTextStream>
#include <QUrl>
#include <QVariant>

namespace QmlDesigner {
#define COLLERR_OK QT_TRANSLATE_NOOP("CollectioParseError", "no error occurred")
#define COLLERR_MAINOBJECT QT_TRANSLATE_NOOP("CollectioParseError", "Document object not found")
#define COLLERR_COLLECTIONNAME QT_TRANSLATE_NOOP("CollectioParseError", "Model name not found")
#define COLLERR_COLLECTIONOBJ QT_TRANSLATE_NOOP("CollectioParseError", "Model is not an object")
#define COLLERR_COLUMNARRAY QT_TRANSLATE_NOOP("CollectioParseError", "Column is not an array")
#define COLLERR_UNKNOWN QT_TRANSLATE_NOOP("CollectioParseError", "Unknown error")

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
public:
    QList<CollectionProperty> properties;
    QList<QJsonArray> dataRecords;
    CollectionReference reference;
    bool isChanged = false;

    bool isValidColumnId(int column) const { return column > -1 && column < properties.size(); }

    bool isValidRowId(int row) const { return row > -1 && row < dataRecords.size(); }
};

inline static bool isValidColorName(const QString &colorName)
{
    return QColor::isValidColorName(colorName);
}

/**
 * @brief getCustomUrl
 * Address = <Url|LocalFile>
 *
 * @param value The input value to be evaluated
 * @param dataType if the value is a valid url, the data type
 * will be stored to this parameter, otherwise, it will be String
 * @param urlResult if the value is a valid url, the address
 * will be stored in this parameter, otherwise it will be empty.
 * @return true if the result is url
 */
static bool getCustomUrl(const QString &value,
                         CollectionDetails::DataType &dataType,
                         QUrl *urlResult = nullptr)
{
    static const QRegularExpression urlRegex{
        "^(?<Address>"
        "(?<Url>https?:\\/\\/"
        "(?:www\\.|(?!www))[A-z0-9][A-z0-9-]+[A-z0-9]\\.[^\\s]{2,}|www\\.[A-z0-9][A-z0-9-]+"
        "[A-z0-9]\\.[^\\s]{2,}|https?:\\/\\/"
        "(?:www\\.|(?!www))[A-z0-9]+\\.[^\\s]{2,}|www\\.[A-z0-9]+\\.[^\\s]{2,})|" // end of Url
        "(?<LocalFile>("
        "?:(?:[A-z]:)|(?:(?:\\\\|\\/){1,2}\\w+)\\$?)(?:(?:\\\\|\\/)(?:\\w[\\w ]*.*))+)" // end of LocalFile
        "){1}$" // end of Address
    };

    const QRegularExpressionMatch match = urlRegex.match(value.trimmed());
    if (match.hasCaptured("Address")) {
        dataType = CollectionDetails::DataType::Url;

        if (urlResult)
            urlResult->setUrl(match.captured("Address"));

        return true;
    }

    if (urlResult)
        urlResult->clear();

    dataType = CollectionDetails::DataType::String;
    return false;
}

/**
 * @brief dataTypeFromString
 * @param value The string value to be evaluated
 * @return Bool, Color, Integer, Real, Url,
 * Image if these types are detected within the non-empty string,
 * Otherwise it returns String.
 * If the value is integer, but it's out of the int range, it will be
 * considered as a Real.
 */
static CollectionDetails::DataType dataTypeFromString(const QString &value)
{
    using DataType = CollectionDetails::DataType;
    static const QRegularExpression validator{
        "(?<boolean>^(?:true|false)$)|"
        "(?<color>^(?:#(?:(?:[0-9a-fA-F]{2}){3,4}|(?:[0-9a-fA-F]){3,4}))$)|"
        "(?<integer>^\\d+$)|"
        "(?<real>^(?:-?(?:0|[1-9]\\d*)?(?:\\.\\d*)?(?<=\\d|\\.)"
        "(?:e-?(?:0|[1-9]\\d*))?|0x[0-9a-f]+)$)"};
    static const int boolIndex = validator.namedCaptureGroups().indexOf("boolean");
    static const int colorIndex = validator.namedCaptureGroups().indexOf("color");
    static const int integerIndex = validator.namedCaptureGroups().indexOf("integer");
    static const int realIndex = validator.namedCaptureGroups().indexOf("real");

    [[maybe_unused]] static const bool allIndexesFound =
        [](const std::initializer_list<int> &captureIndexes) {
            QTC_ASSERT(Utils::allOf(captureIndexes, [](int val) { return val > -1; }), return false);
            return true;
        }({boolIndex, colorIndex, integerIndex, realIndex});

    if (value.isEmpty())
        return DataType::String;

    const QString trimmedValue = value.trimmed();
    QRegularExpressionMatch match = validator.match(trimmedValue);

    if (match.hasCaptured(boolIndex))
        return DataType::Boolean;
    if (match.hasCaptured(colorIndex))
        return DataType::Color;
    if (match.hasCaptured(integerIndex)) {
        bool isInt = false;
        trimmedValue.toInt(&isInt);
        return isInt ? DataType::Integer : DataType::Real;
    }
    if (match.hasCaptured(realIndex))
        return DataType::Real;

    DataType urlType;
    if (getCustomUrl(trimmedValue, urlType))
        return urlType;

    return DataType::String;
}

static CollectionProperty::DataType dataTypeFromJsonValue(const QJsonValue &value)
{
    using DataType = CollectionDetails::DataType;
    using JsonType = QJsonValue::Type;

    switch (value.type()) {
    case JsonType::Null:
    case JsonType::Undefined:
        return DataType::String;
    case JsonType::Bool:
        return DataType::Boolean;
    case JsonType::Double: {
        if (qFuzzyIsNull(std::remainder(value.toDouble(), 1)))
            return DataType::Integer;
        return DataType::Real;
    }
    case JsonType::String:
        return dataTypeFromString(value.toString());
    default:
        return DataType::String;
    }
}

static QList<CollectionProperty> getColumnsFromImportedJsonArray(const QJsonArray &importedArray)
{
    using DataType = CollectionDetails::DataType;

    QHash<QString, int> resultSet;
    QList<CollectionProperty> result;

    for (const QJsonValue &value : importedArray) {
        if (value.isObject()) {
            const QJsonObject object = value.toObject();
            QJsonObject::ConstIterator element = object.constBegin();
            const QJsonObject::ConstIterator stopItem = object.constEnd();

            while (element != stopItem) {
                const QString propertyName = element.key();
                if (resultSet.contains(propertyName)) {
                    CollectionProperty &property = result[resultSet.value(propertyName)];
                    if (property.type == DataType::Integer) {
                        const DataType currentCellDataType = dataTypeFromJsonValue(element.value());
                        if (currentCellDataType == DataType::Real)
                            property.type = currentCellDataType;
                    }
                } else {
                    result.append({propertyName, dataTypeFromJsonValue(element.value())});
                    resultSet.insert(propertyName, resultSet.size());
                }
                ++element;
            }
        }
    }

    return result;
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
    case DataType::Url:
    case DataType::Image:
        return variantValue.value<QUrl>();
    default:
        return variantValue;
    }
}

static QJsonValue variantToJsonValue(
    const QVariant &variant, CollectionDetails::DataType type = CollectionDetails::DataType::String)
{
    using DataType = CollectionDetails::DataType;

    switch (type) {
    case DataType::Boolean:
        return variant.toBool();
    case DataType::Real:
        return variant.toDouble();
    case DataType::Integer:
        return variant.toInt();
    case DataType::Image:
    case DataType::String:
    case DataType::Color:
    case DataType::Url:
    default:
        return variant.toString();
    }
}

inline static bool isEmptyJsonValue(const QJsonValue &value)
{
    return value.isNull() || value.isUndefined() || (value.isString() && value.toString().isEmpty());
}

QStringList csvReadLine(const QString &line)
{
    static const QRegularExpression lineRegex{
        "(?:,\\\"|^\\\")(?<value>\\\"\\\"|[\\w\\W]*?)(?=\\\",|\\\"$)"
        "|(?:,(?!\\\")|^(?!\\\"))(?<quote>[^,]*?)(?=$|,)|(\\\\r\\\\n|\\\\n)"};
    static const int valueIndex = lineRegex.namedCaptureGroups().indexOf("value");
    static const int quoteIndex = lineRegex.namedCaptureGroups().indexOf("quote");
    Q_ASSERT(valueIndex > 0 && quoteIndex > 0);

    QStringList result;
    QRegularExpressionMatchIterator iterator = lineRegex.globalMatch(line, 0);
    while (iterator.hasNext()) {
        const QRegularExpressionMatch match = iterator.next();

        if (match.hasCaptured(valueIndex))
            result.append(match.captured(valueIndex));
        else if (match.hasCaptured(quoteIndex))
            result.append(match.captured(quoteIndex));
    }
    return result;
}

class PropertyOrderFinder : public QmlJS::AST::Visitor
{
public:
    static QStringList parse(const QString &jsonContent)
    {
        PropertyOrderFinder finder;
        QmlJS::Document::MutablePtr jsonDoc = QmlJS::Document::create(Utils::FilePath::fromString(
                                                                          "<expression>"),
                                                                      QmlJS::Dialect::Json);

        jsonDoc->setSource(jsonContent);
        jsonDoc->parseJavaScript();

        if (!jsonDoc->isParsedCorrectly())
            return {};

        jsonDoc->ast()->accept(&finder);
        return finder.m_orderedList;
    }

protected:
    bool visit(QmlJS::AST::PatternProperty *patternProperty) override
    {
        const QString propertyName = patternProperty->name->asString();
        if (!m_propertySet.contains(propertyName)) {
            m_propertySet.insert(propertyName);
            m_orderedList.append(propertyName);
        }
        return true;
    }

    void throwRecursionDepthError() override
    {
        qWarning() << Q_FUNC_INFO << __LINE__ << "Recursion depth error";
    };

private:
    QSet<QString> m_propertySet;
    QStringList m_orderedList;
};

QString CollectionParseError::errorString() const
{
    switch (errorNo) {
    case NoError:
        return COLLERR_OK;
    case MainObjectMissing:
        return COLLERR_MAINOBJECT;
    case CollectionNameNotFound:
        return COLLERR_COLLECTIONNAME;
    case CollectionIsNotObject:
        return COLLERR_COLLECTIONOBJ;
    case ColumnsBlockIsNotArray:
        return COLLERR_COLUMNARRAY;
    case UnknownError:
    default:
        return COLLERR_UNKNOWN;
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

void CollectionDetails::resetData(const QJsonDocument &localDocument,
                                  const QString &collectionToImport,
                                  CollectionParseError *error)
{
    CollectionDetails importedCollection = fromLocalJson(localDocument, collectionToImport, error);
    d->properties.swap(importedCollection.d->properties);
    d->dataRecords.swap(importedCollection.d->dataRecords);
}

CollectionDetails::CollectionDetails(const CollectionDetails &other) = default;

CollectionDetails::~CollectionDetails() = default;

void CollectionDetails::insertColumn(const QString &propertyName,
                                     int colIdx,
                                     const QVariant &defaultValue,
                                     DataType type)
{
    if (containsPropertyName(propertyName))
        return;

    CollectionProperty property = {propertyName, type};
    if (d->isValidColumnId(colIdx)) {
        d->properties.insert(colIdx, property);
    } else {
        colIdx = d->properties.size();
        d->properties.append(property);
    }

    const QJsonValue defaultJsonValue = QJsonValue::fromVariant(defaultValue);
    for (QJsonArray &record : d->dataRecords)
        record.insert(colIdx, defaultJsonValue);

    markChanged();
}

bool CollectionDetails::removeColumns(int colIdx, int count)
{
    if (!d->isValidColumnId(colIdx))
        return false;

    int maxCount = d->properties.count() - colIdx;
    count = std::min(maxCount, count);

    if (count < 1)
        return false;

    d->properties.remove(colIdx, count);

    for (QJsonArray &record : d->dataRecords) {
        QJsonArray newElement;

        auto elementItr = record.constBegin();
        auto elementEnd = elementItr + colIdx;
        while (elementItr != elementEnd)
            newElement.append(*(elementItr++));

        elementItr += count;
        elementEnd = record.constEnd();

        while (elementItr != elementEnd)
            newElement.append(*(elementItr++));

        record = newElement;
    }

    markChanged();

    return true;
}

void CollectionDetails::insertEmptyRows(int row, int count)
{
    if (count < 1)
        return;

    row = qBound(0, row, rows());

    insertRecords({}, row, count);

    markChanged();
}

bool CollectionDetails::removeRows(int row, int count)
{
    if (!d->isValidRowId(row))
        return false;

    int maxCount = d->dataRecords.count() - row;
    count = std::min(maxCount, count);

    if (count < 1)
        return false;

    d->dataRecords.remove(row, count);
    markChanged();
    return true;
}

bool CollectionDetails::setPropertyValue(int row, int column, const QVariant &value)
{
    if (!d->isValidRowId(row) || !d->isValidColumnId(column))
        return false;

    QVariant currentValue = data(row, column);
    if (value == currentValue)
        return false;

    QJsonArray &record = d->dataRecords[row];
    record.replace(column, variantToJsonValue(value, typeAt(column)));
    markChanged();
    return true;
}

bool CollectionDetails::setPropertyName(int column, const QString &value)
{
    if (!d->isValidColumnId(column))
        return false;

    const CollectionProperty &oldProperty = d->properties.at(column);
    if (oldProperty.name == value)
        return false;

    d->properties.replace(column, {value, oldProperty.type});

    markChanged();
    return true;
}

bool CollectionDetails::setPropertyType(int column, DataType type)
{
    if (!d->isValidColumnId(column))
        return false;

    bool changed = false;
    CollectionProperty &property = d->properties[column];
    if (property.type != type)
        changed = true;

    const DataType formerType = property.type;
    property.type = type;

    for (QJsonArray &rowData : d->dataRecords) {
        if (column < rowData.size()) {
            const QJsonValue value = rowData.at(column);
            const QVariant properTypedValue = valueToVariant(value, formerType);
            const QJsonValue properTypedJsonValue = variantToJsonValue(properTypedValue, type);
            rowData.replace(column, properTypedJsonValue);
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

QVariant CollectionDetails::data(int row, int column) const
{
    if (!d->isValidRowId(row))
        return {};

    if (!d->isValidColumnId(column))
        return {};

    const QJsonValue cellValue = d->dataRecords.at(row).at(column);

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

    const QJsonValue cellData = d->dataRecords.at(row).at(column);
    return dataTypeFromJsonValue(cellData);
}

DataTypeWarning::Warning CollectionDetails::cellWarningCheck(int row, int column) const
{
    const QJsonValue cellValue = d->dataRecords.at(row).at(column);

    const DataType columnType = typeAt(column);
    const DataType cellType = typeAt(row, column);

    if (isEmptyJsonValue(cellValue))
        return DataTypeWarning::Warning::None;

    if ((columnType == DataType::String || columnType == DataType::Real) && cellType == DataType::Integer)
        return DataTypeWarning::Warning::None;

    if ((columnType == DataType::Url || columnType == DataType::Image) && cellType == DataType::String)
        return DataTypeWarning::Warning::None;

    if (columnType != cellType)
        return DataTypeWarning::Warning::CellDataTypeMismatch;

    return DataTypeWarning::Warning::None;
}

bool CollectionDetails::containsPropertyName(const QString &propertyName) const
{
    return Utils::anyOf(d->properties, [&propertyName](const CollectionProperty &property) {
        return property.name == propertyName;
    });
}

bool CollectionDetails::hasValidReference() const
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
    return d->dataRecords.size();
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

void CollectionDetails::resetReference(const CollectionReference &reference)
{
    if (d->reference != reference) {
        d->reference = reference;
        markChanged();
    }
}

QString CollectionDetails::toJson() const
{
    QJsonArray exportedArray;
    const int propertyCount = d->properties.count();

    for (const QJsonArray &record : std::as_const(d->dataRecords)) {
        const int valueCount = std::min(int(record.count()), propertyCount);

        QJsonObject exportedElement;
        for (int i = 0; i < valueCount; ++i) {
            const QJsonValue &value = record.at(i);
            if (isEmptyJsonValue(value))
                exportedElement.insert(d->properties.at(i).name, QJsonValue::Null);
            else
                exportedElement.insert(d->properties.at(i).name, value);
        }

        exportedArray.append(exportedElement);
    }

    return QString::fromUtf8(QJsonDocument(exportedArray).toJson());
}

QString CollectionDetails::toCsv() const
{
    QString content;

    auto gotoNextLine = [&content]() {
        if (content.size() && content.back() == ',')
            content.back() = '\n';
        else
            content += "\n";
    };

    const int propertyCount = d->properties.count();
    if (propertyCount <= 0)
        return "";

    for (const CollectionProperty &property : std::as_const(d->properties))
        content += property.name + ',';

    gotoNextLine();

    for (const QJsonArray &record : std::as_const(d->dataRecords)) {
        const int valueCount = std::min(int(record.count()), propertyCount);
        int i = 0;
        for (; i < valueCount; ++i) {
            const QJsonValue &value = record.at(i);

            if (value.isDouble())
                content += QString::number(value.toDouble()) + ',';
            else if (value.isBool())
                content += value.toBool() ? QString("true,") : QString("false,");
            else
                content += value.toString() + ',';
        }

        for (; i < propertyCount; ++i)
            content += ',';

        gotoNextLine();
    }

    return content;
}

QJsonObject CollectionDetails::toLocalJson() const
{
    QJsonObject collectionObject;
    QJsonArray columnsArray;
    QJsonArray dataArray;

    for (const CollectionProperty &property : std::as_const(d->properties)) {
        QJsonObject columnObject;
        columnObject.insert("name", property.name);
        columnObject.insert("type", CollectionDataTypeModel::dataTypeToString(property.type));
        columnsArray.append(columnObject);
    }

    for (const QJsonArray &record : std::as_const(d->dataRecords))
        dataArray.append(record);

    collectionObject.insert("columns", columnsArray);
    collectionObject.insert("data", dataArray);

    return collectionObject;
}

void CollectionDetails::registerDeclarativeType()
{
    typedef CollectionDetails::DataType DataType;
    qRegisterMetaType<DataType>("DataType");
    qmlRegisterUncreatableType<CollectionDetails>("CollectionDetails", 1, 0, "DataType", "Enum type");

    qRegisterMetaType<DataTypeWarning::Warning>("Warning");
    qmlRegisterUncreatableType<DataTypeWarning>("CollectionDetails", 1, 0, "Warning", "Enum type");
}

CollectionDetails CollectionDetails::fromImportedCsv(const QByteArray &document,
                                                     const bool &firstRowIsHeader)
{
    QStringList headers;
    QJsonArray importedArray;

    QTextStream stream(document);
    stream.setEncoding(QStringConverter::Latin1);

    if (firstRowIsHeader && !stream.atEnd()) {
        headers = Utils::transform(csvReadLine(stream.readLine()),
                                   [](const QString &value) -> QString { return value.trimmed(); });
    }

    while (!stream.atEnd()) {
        const QStringList recordDataList = csvReadLine(stream.readLine());
        int column = -1;
        QJsonObject recordData;
        for (const QString &cellData : recordDataList) {
            if (++column == headers.size()) {
                QString proposalName;
                int proposalId = column;
                do
                    proposalName = QString("Column %1").arg(++proposalId);
                while (headers.contains(proposalName));
                headers.append(proposalName);
            }
            recordData.insert(headers.at(column), cellData);
        }
        importedArray.append(recordData);
    }

    return fromImportedJson(importedArray, headers);
}

CollectionDetails CollectionDetails::fromImportedJson(const QByteArray &json, QJsonParseError *error)
{
    QJsonArray importedCollection;
    auto refineJsonArray = [](const QJsonArray &array) -> QJsonArray {
        QJsonArray resultArray;
        for (const QJsonValue &collectionData : array) {
            if (collectionData.isObject()) {
                QJsonObject rowObject = collectionData.toObject();
                const QStringList rowKeys = rowObject.keys();
                for (const QString &key : rowKeys) {
                    const QJsonValue cellValue = rowObject.value(key);
                    if (cellValue.isArray())
                        rowObject.remove(key);
                }
                resultArray.push_back(rowObject);
            }
        }
        return resultArray;
    };

    QJsonParseError parseError;
    QJsonDocument document = QJsonDocument::fromJson(json, &parseError);
    if (error)
        *error = parseError;

    if (parseError.error != QJsonParseError::NoError)
        return CollectionDetails{};

    if (document.isArray()) {
        importedCollection = refineJsonArray(document.array());
    } else if (document.isObject()) {
        QJsonObject documentObject = document.object();
        const QStringList mainKeys = documentObject.keys();

        bool arrayFound = false;
        for (const QString &key : mainKeys) {
            const QJsonValue value = documentObject.value(key);
            if (value.isArray()) {
                arrayFound = true;
                importedCollection = refineJsonArray(value.toArray());
                break;
            }
        }

        if (!arrayFound) {
            QJsonObject singleObject;
            for (const QString &key : mainKeys) {
                const QJsonValue value = documentObject.value(key);

                if (!value.isObject())
                    singleObject.insert(key, value);
            }
            importedCollection.push_back(singleObject);
        }
    }

    return fromImportedJson(importedCollection, PropertyOrderFinder::parse(QLatin1String(json)));
}

CollectionDetails CollectionDetails::fromLocalJson(const QJsonDocument &document,
                                                   const QString &collectionName,
                                                   CollectionParseError *error)
{
    auto setError = [&error](CollectionParseError::ParseError parseError) {
        if (error)
            error->errorNo = parseError;
    };

    setError(CollectionParseError::NoError);

    if (document.isObject()) {
        QJsonObject collectionMap = document.object();
        if (collectionMap.contains(collectionName)) {
            QJsonValue collectionValue = collectionMap.value(collectionName);
            if (collectionValue.isObject())
                return fromLocalCollection(collectionValue.toObject());
            else
                setError(CollectionParseError::CollectionIsNotObject);
        } else {
            setError(CollectionParseError::CollectionNameNotFound);
        }
    } else {
        setError(CollectionParseError::MainObjectMissing);
    }

    return CollectionDetails{};
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

void CollectionDetails::insertRecords(const QJsonArray &record, int idx, int count)
{
    if (count < 1)
        return;

    QJsonArray localRecord;
    const int columnsCount = columns();
    for (int i = 0; i < columnsCount; i++) {
        const QJsonValue originalCellData = record.at(i);
        if (originalCellData.isArray())
            localRecord.append({});
        else
            localRecord.append(originalCellData);
    }

    if (idx > d->dataRecords.size() || idx < 0)
        idx = d->dataRecords.size();

    d->dataRecords.insert(idx, count, localRecord);
}

CollectionDetails CollectionDetails::fromImportedJson(const QJsonArray &importedArray,
                                                      const QStringList &propertyPriority)
{
    QList<CollectionProperty> columnData = getColumnsFromImportedJsonArray(importedArray);
    if (!propertyPriority.isEmpty()) {
        QMap<QString, int> priorityMap;
        for (const QString &propertyName : propertyPriority) {
            if (!priorityMap.contains(propertyName))
                priorityMap.insert(propertyName, priorityMap.size());
        }
        const int lowestPriority = priorityMap.size();

        Utils::sort(columnData, [&](const CollectionProperty &a, const CollectionProperty &b) {
            return priorityMap.value(a.name, lowestPriority)
                   < priorityMap.value(b.name, lowestPriority);
        });
    }

    QList<QJsonArray> localJsonArray;
    for (const QJsonValue &importedRowValue : importedArray) {
        QJsonObject importedRowObject = importedRowValue.toObject();
        QJsonArray localRow;
        for (const CollectionProperty &property : columnData)
            localRow.append(importedRowObject.value(property.name));
        localJsonArray.append(localRow);
    }
    CollectionDetails result;
    result.d->properties = columnData;
    result.d->dataRecords = localJsonArray;
    result.markSaved();

    return result;
}

CollectionDetails CollectionDetails::fromLocalCollection(const QJsonObject &localCollection,
                                                         CollectionParseError *error)
{
    auto setError = [&error](CollectionParseError::ParseError parseError) {
        if (error)
            error->errorNo = parseError;
    };

    CollectionDetails result;
    setError(CollectionParseError::NoError);

    if (localCollection.contains("columns")) {
        const QJsonValue columnsValue = localCollection.value("columns");
        if (columnsValue.isArray()) {
            const QJsonArray columns = columnsValue.toArray();
            for (const QJsonValue &columnValue : columns) {
                if (columnValue.isObject()) {
                    const QJsonObject column = columnValue.toObject();
                    const QString columnName = column.value("name").toString();
                    if (!columnName.isEmpty()) {
                        result.insertColumn(columnName,
                                            -1,
                                            {},
                                            CollectionDataTypeModel::dataTypeFromString(
                                                column.value("type").toString()));
                    }
                }
            }

            if (int columnsCount = result.columns()) {
                const QJsonArray dataRecords = localCollection.value("data").toArray();
                for (const QJsonValue &dataRecordValue : dataRecords) {
                    QJsonArray dataRecord = dataRecordValue.toArray();
                    while (dataRecord.count() > columnsCount)
                        dataRecord.removeLast();

                    result.insertRecords(dataRecord);
                }
            }
        } else {
            setError(CollectionParseError::ColumnsBlockIsNotArray);
            return result;
        }
    }

    return result;
}

} // namespace QmlDesigner
