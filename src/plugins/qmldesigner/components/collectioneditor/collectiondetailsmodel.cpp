// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "collectiondetailsmodel.h"

#include "collectioneditorconstants.h"
#include "collectioneditorutils.h"
#include "modelnode.h"

#include <coreplugin/editormanager/editormanager.h>

#include <utils/fileutils.h>
#include <utils/qtcassert.h>
#include <utils/textfileformat.h>

#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>

namespace {

QStringList getJsonHeaders(const QJsonArray &collectionArray)
{
    QSet<QString> resultSet;
    QList<QString> result;

    for (const QJsonValue &value : collectionArray) {
        if (value.isObject()) {
            const QJsonObject object = value.toObject();
            QJsonObject::ConstIterator element = object.constBegin();
            const QJsonObject::ConstIterator stopItem = object.constEnd();

            while (element != stopItem) {
                const QString property = element.key();
                if (!resultSet.contains(property)) {
                    result.append(property);
                    resultSet.insert(property);
                }
                ++element;
            }
        }
    }

    return result;
}

class CollectionDataTypeHelper
{
public:
    using DataType = QmlDesigner::CollectionDetails::DataType;

    static QString typeToString(DataType dataType)
    {
        static const QHash<DataType, QString> typeStringHash = typeToStringHash();
        return typeStringHash.value(dataType);
    }

    static DataType typeFromString(const QString &dataType)
    {
        static const QHash<QString, DataType> stringTypeHash = stringToTypeHash();
        return stringTypeHash.value(dataType, DataType::Unknown);
    }

    static QStringList typesStringList()
    {
        static const QStringList typesList = orderedTypeNames();
        return typesList;
    }

private:
    CollectionDataTypeHelper() = delete;

    static QHash<DataType, QString> typeToStringHash()
    {
        return {
            {DataType::Unknown, "Unknown"},
            {DataType::String, "String"},
            {DataType::Url, "Url"},
            {DataType::Real, "Real"},
            {DataType::Integer, "Integer"},
            {DataType::Boolean, "Boolean"},
            {DataType::Image, "Image"},
            {DataType::Color, "Color"},
        };
    }

    static QHash<QString, DataType> stringToTypeHash()
    {
        QHash<QString, DataType> stringTypeHash;
        const QHash<DataType, QString> typeStringHash = typeToStringHash();
        for (const auto &transferItem : typeStringHash.asKeyValueRange())
            stringTypeHash.insert(transferItem.second, transferItem.first);

        return stringTypeHash;
    }

    static QStringList orderedTypeNames()
    {
        const QList<DataType> orderedtypes{
            DataType::String,
            DataType::Integer,
            DataType::Real,
            DataType::Image,
            DataType::Color,
            DataType::Url,
            DataType::Boolean,
            DataType::Unknown,
        };

        QStringList orderedNames;
        QHash<DataType, QString> typeStringHash = typeToStringHash();

        for (const DataType &type : orderedtypes)
            orderedNames.append(typeStringHash.take(type));

        Q_ASSERT(typeStringHash.isEmpty());
        return orderedNames;
    }
};

} // namespace

namespace QmlDesigner {

CollectionDetailsModel::CollectionDetailsModel(QObject *parent)
    : QAbstractTableModel(parent)
{
    connect(this, &CollectionDetailsModel::modelReset, this, &CollectionDetailsModel::updateEmpty);
    connect(this, &CollectionDetailsModel::rowsInserted, this, &CollectionDetailsModel::updateEmpty);
    connect(this, &CollectionDetailsModel::rowsRemoved, this, &CollectionDetailsModel::updateEmpty);
}

QHash<int, QByteArray> CollectionDetailsModel::roleNames() const
{
    static QHash<int, QByteArray> roles;
    if (roles.isEmpty()) {
        roles.insert(QAbstractTableModel::roleNames());
        roles.insert(SelectedRole, "itemSelected");
        roles.insert(DataTypeRole, "dataType");
        roles.insert(ColumnDataTypeRole, "columnType");
        roles.insert(DataTypeWarningRole, "dataTypeWarning");
    }
    return roles;
}

int CollectionDetailsModel::rowCount([[maybe_unused]] const QModelIndex &parent) const
{
    return m_currentCollection.rows();
}

int CollectionDetailsModel::columnCount([[maybe_unused]] const QModelIndex &parent) const
{
    return m_currentCollection.columns();
}

QVariant CollectionDetailsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return {};

    if (role == SelectedRole)
        return (index.column() == m_selectedColumn || index.row() == m_selectedRow);

    if (role == DataTypeRole)
        return QVariant::fromValue(m_currentCollection.typeAt(index.row(), index.column()));

    if (role == ColumnDataTypeRole)
        return QVariant::fromValue(m_currentCollection.typeAt(index.column()));

    if (role == Qt::EditRole)
        return m_currentCollection.data(index.row(), index.column());

    if (role == DataTypeWarningRole )
        return QVariant::fromValue(m_currentCollection.cellWarningCheck(index.row(), index.column()));

    return m_currentCollection.data(index.row(), index.column()).toString();
}

bool CollectionDetailsModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid())
        return {};

    if (role == Qt::EditRole) {
        DataTypeWarning::Warning prevWarning = m_currentCollection.cellWarningCheck(index.row(), index.column());
        bool changed = m_currentCollection.setPropertyValue(index.row(), index.column(), value);

        if (changed) {
            QList<int> roles = {Qt::DisplayRole, Qt::EditRole};

            if (prevWarning != m_currentCollection.cellWarningCheck(index.row(), index.column()))
                roles << DataTypeWarningRole;

            emit dataChanged(index, index, roles);
        }

        return true;
    }

    return false;
}

bool CollectionDetailsModel::setHeaderData(int section,
                                          Qt::Orientation orientation,
                                          const QVariant &value,
                                          [[maybe_unused]] int role)
{
    if (orientation == Qt::Vertical)
        return false;

    bool headerChanged = m_currentCollection.setPropertyName(section, value.toString());
    if (headerChanged)
        emit this->headerDataChanged(orientation, section, section);

    return headerChanged;
}

bool CollectionDetailsModel::insertRows(int row, int count, [[maybe_unused]] const QModelIndex &parent)
{
    if (count < 1)
        return false;

    row = qBound(0, row, rowCount());

    beginResetModel();
    m_currentCollection.insertEmptyElements(row, count);
    endResetModel();

    selectRow(row);
    return true;
}

bool CollectionDetailsModel::removeColumns(int column, int count, const QModelIndex &parent)
{
    if (column < 0 || column >= columnCount(parent) || count < 1)
        return false;

    count = std::min(count, columnCount(parent) - column);
    beginRemoveColumns(parent, column, column + count - 1);
    bool columnsRemoved = m_currentCollection.removeColumns(column, count);
    endRemoveColumns();

    if (!columnCount(parent))
        removeRows(0, rowCount(parent), parent);

    int nextColumn = column - 1;
    if (nextColumn < 0 && columnCount(parent) > 0)
        nextColumn = 0;

    selectColumn(nextColumn);

    ensureSingleCell();
    return columnsRemoved;
}

bool CollectionDetailsModel::removeRows(int row, int count, const QModelIndex &parent)
{
    if (row < 0 || row >= rowCount(parent) || count < 1)
        return false;

    count = std::min(count, rowCount(parent) - row);
    beginRemoveRows(parent, row, row + count - 1);
    bool rowsRemoved = m_currentCollection.removeElements(row, count);
    endRemoveRows();

    ensureSingleCell();
    return rowsRemoved;
}

Qt::ItemFlags CollectionDetailsModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return {};

    return {Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable};
}

QVariant CollectionDetailsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal) {
        if (role == DataTypeRole)
            return CollectionDataTypeHelper::typeToString(m_currentCollection.typeAt(section));
        else
            return m_currentCollection.propertyAt(section);
    }

    if (orientation == Qt::Vertical)
        return section + 1;

    return {};
}

CollectionDetails::DataType CollectionDetailsModel::propertyDataType(int column) const
{
    return m_currentCollection.typeAt(column);
}

int CollectionDetailsModel::selectedColumn() const
{
    return m_selectedColumn;
}

int CollectionDetailsModel::selectedRow() const
{
    return m_selectedRow;
}

QString CollectionDetailsModel::propertyName(int column) const
{
    return m_currentCollection.propertyAt(column);
}

QString CollectionDetailsModel::propertyType(int column) const
{
    return CollectionDataTypeHelper::typeToString(m_currentCollection.typeAt(column));
}

bool CollectionDetailsModel::isPropertyAvailable(const QString &name)
{
    return m_currentCollection.containsPropertyName(name);
}

bool CollectionDetailsModel::addColumn(int column, const QString &name, const QString &propertyType)
{
    if (m_currentCollection.containsPropertyName(name))
        return false;

    if (column < 0 || column > columnCount())
        column = columnCount();

    beginInsertColumns({}, column, column);
    m_currentCollection.insertColumn(name,
                                     column,
                                     {},
                                     CollectionDataTypeHelper::typeFromString(propertyType));
    endInsertColumns();
    return m_currentCollection.containsPropertyName(name);
}

bool CollectionDetailsModel::selectColumn(int section)
{
    if (m_selectedColumn == section)
        return false;

    const int columns = columnCount();

    if (section >= columns)
        section = columns - 1;

    selectRow(-1);

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

bool CollectionDetailsModel::renameColumn(int section, const QString &newValue)
{
    return setHeaderData(section, Qt::Horizontal, newValue);
}

bool CollectionDetailsModel::setPropertyType(int column, const QString &newValue)
{
    bool changed = m_currentCollection.setPropertyType(column,
                                                       CollectionDataTypeHelper::typeFromString(
                                                           newValue));
    if (changed) {
        emit headerDataChanged(Qt::Horizontal, column, column);
        emit dataChanged(
            index(0, column),
            index(rowCount() - 1, column),
            {Qt::DisplayRole, Qt::EditRole, DataTypeRole, DataTypeWarningRole, ColumnDataTypeRole});
    }

    return changed;
}

bool CollectionDetailsModel::selectRow(int row)
{
    if (m_selectedRow == row)
        return false;

    const int rows = rowCount();

    if (row >= rows)
        row = rows - 1;

    selectColumn(-1);

    const int columns = columnCount();
    const int previousRow = m_selectedRow;

    m_selectedRow = row;
    emit this->selectedRowChanged(m_selectedRow);

    auto notifySelectedDataChanged = [this, rows, columns](int notifyingRow) {
        if (notifyingRow > -1 && notifyingRow < rows && columns)
            emit dataChanged(index(notifyingRow, 0), index(notifyingRow, columns - 1), {SelectedRole});
    };

    notifySelectedDataChanged(previousRow);
    notifySelectedDataChanged(m_selectedRow);

    return true;
}

void CollectionDetailsModel::deselectAll()
{
    selectColumn(-1);
    selectRow(-1);
}

QStringList CollectionDetailsModel::typesList()
{
    return CollectionDataTypeHelper::typesStringList();
}

void CollectionDetailsModel::loadCollection(const ModelNode &sourceNode, const QString &collection)
{
    QString fileName = CollectionEditorUtils::getSourceCollectionPath(sourceNode);

    CollectionReference newReference{sourceNode, collection};
    bool alreadyOpen = m_openedCollections.contains(newReference);

    if (alreadyOpen) {
        if (m_currentCollection.reference() != newReference) {
            deselectAll();
            beginResetModel();
            switchToCollection(newReference);
            ensureSingleCell();
            endResetModel();
        }
    } else {
        deselectAll();
        switchToCollection(newReference);
        if (sourceNode.type() == CollectionEditorConstants::JSONCOLLECTIONMODEL_TYPENAME)
            loadJsonCollection(fileName, collection);
        else if (sourceNode.type() == CollectionEditorConstants::CSVCOLLECTIONMODEL_TYPENAME)
            loadCsvCollection(fileName, collection);
    }
}

bool CollectionDetailsModel::saveDataStoreCollections()
{
    const ModelNode node = m_currentCollection.reference().node;
    const Utils::FilePath path = CollectionEditorUtils::dataStoreJsonFilePath();
    Utils::FileReader fileData;
    Utils::FileSaver sourceFile(path);

    if (!fileData.fetch(path)) {
        qWarning() << Q_FUNC_INFO << "Cannot read the json file:" << fileData.errorString();
        return false;
    }

    QJsonParseError jpe;
    QJsonDocument document = QJsonDocument::fromJson(fileData.data(), &jpe);

    if (jpe.error == QJsonParseError::NoError) {
        QJsonObject obj = document.object();

        QList<CollectionDetails> collectionsToBeSaved;
        for (CollectionDetails &openedCollection : m_openedCollections) {
            const CollectionReference reference = openedCollection.reference();
            if (reference.node == node) {
                obj.insert(reference.name, openedCollection.getCollectionAsJsonArray());
                collectionsToBeSaved << openedCollection;
            }
        }

        document.setObject(obj);
        bool saved = sourceFile.write(document.toJson());
        saved &= sourceFile.finalize();

        if (saved) {
            const CollectionReference currentReference = m_currentCollection.reference();
            for (CollectionDetails &collection : collectionsToBeSaved) {
                collection.markSaved();
                const CollectionReference reference = collection.reference();
                if (reference != currentReference)
                    closeCollectionIfSaved(reference);
            }
            return true;
        }
    }
    return false;
}

bool CollectionDetailsModel::exportCollection(const QUrl &url)
{
    using Core::EditorManager;
    using Utils::FilePath;
    using Utils::TextFileFormat;

    QTC_ASSERT(m_currentCollection.isValid(), return false);

    bool saved = false;
    const FilePath filePath = FilePath::fromUserInput(url.isLocalFile() ? url.toLocalFile()
                                                                        : url.toString());
    const QString saveFormat = filePath.toFileInfo().suffix().toLower();
    const QString content = saveFormat == "csv" ? m_currentCollection.getCollectionAsCsvString()
                                                : m_currentCollection.getCollectionAsJsonString();

    TextFileFormat textFileFormat;
    textFileFormat.codec = EditorManager::defaultTextCodec();
    textFileFormat.lineTerminationMode = EditorManager::defaultLineEnding();
    QString errorMessage;
    saved = textFileFormat.writeFile(filePath, content, &errorMessage);

    if (!saved)
        qWarning() << Q_FUNC_INFO << "Unable to write file" << errorMessage;

    return saved;
}

void CollectionDetailsModel::updateEmpty()
{
    bool isEmptyNow = rowCount() == 0;
    if (m_isEmpty != isEmptyNow) {
        m_isEmpty = isEmptyNow;
        emit isEmptyChanged(m_isEmpty);
    }
}

void CollectionDetailsModel::switchToCollection(const CollectionReference &collection)
{
    if (m_currentCollection.reference() == collection)
        return;

    closeCurrentCollectionIfSaved();

    if (!m_openedCollections.contains(collection))
        m_openedCollections.insert(collection, CollectionDetails(collection));

    m_currentCollection = m_openedCollections.value(collection);

    setCollectionName(collection.name);
}

void CollectionDetailsModel::closeCollectionIfSaved(const CollectionReference &collection)
{
    if (!m_openedCollections.contains(collection))
        return;

    const CollectionDetails &collectionDetails = m_openedCollections.value(collection);

    if (!collectionDetails.isChanged())
        m_openedCollections.remove(collection);
}

void CollectionDetailsModel::closeCurrentCollectionIfSaved()
{
    if (m_currentCollection.isValid()) {
        closeCollectionIfSaved(m_currentCollection.reference());
        m_currentCollection = CollectionDetails{};
    }
}

void CollectionDetailsModel::loadJsonCollection(const QString &source, const QString &collection)
{
    using CollectionEditorConstants::SourceFormat;

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
    ensureSingleCell();
    endResetModel();
}

void CollectionDetailsModel::loadCsvCollection(const QString &source,
                                              [[maybe_unused]] const QString &collectionName)
{
    using CollectionEditorConstants::SourceFormat;

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

    for (const QString &header : std::as_const(headers)) {
        for (QJsonObject &element: elements) {
            QVariant variantValue;
            if (element.contains(header)) {
                variantValue = variantFromString(element.value(header).toString());
                element[header] = variantValue.toJsonValue();

                if (variantValue.isValid())
                    break;
            }
        }
    }

    SourceFormat sourceFormat = csvFileIsOk ? SourceFormat::Csv : SourceFormat::Unknown;
    beginResetModel();
    m_currentCollection.resetDetails(headers, elements, sourceFormat);
    ensureSingleCell();
    endResetModel();
}

void CollectionDetailsModel::ensureSingleCell()
{
    if (!m_currentCollection.isValid())
        return;

    if (!columnCount())
        addColumn(0, "Column 1", "String");

    if (!rowCount())
        insertRow(0);

    updateEmpty();
}

QVariant CollectionDetailsModel::variantFromString(const QString &value)
{
    constexpr QStringView typesPattern{u"(?<boolean>^(?:true|false)$)|"
                                       u"(?<number>^(?:-?(?:0|[1-9]\\d*)?(?:\\.\\d*)?(?<=\\d|\\.)"
                                       u"(?:e-?(?:0|[1-9]\\d*))?|0x[0-9a-f]+)$)|"
                                       u"(?<color>^(?:#(?:(?:[0-9a-fA-F]{2}){3,4}|"
                                       u"(?:[0-9a-fA-F]){3,4}))$)|"
                                       u"(?<string>[A-Za-z][A-Za-z0-9_ -]*)"};
    static QRegularExpression validator(typesPattern.toString());
    const QString trimmedValue = value.trimmed();
    QRegularExpressionMatch match = validator.match(trimmedValue);
    QVariant variantValue = value;

    if (value.isEmpty())
        return QVariant();
    if (!match.captured(u"boolean").isEmpty())
        return variantValue.toBool();
    if (!match.captured(u"number").isEmpty())
        return variantValue.toDouble();
    if (!match.captured(u"color").isEmpty())
        return variantValue.value<QColor>();
    if (!match.captured(u"string").isEmpty())
        return variantValue.toString();

    return QVariant::fromValue(value);
}

void CollectionDetailsModel::setCollectionName(const QString &newCollectionName)
{
    if (m_collectionName != newCollectionName) {
        m_collectionName = newCollectionName;
        emit this->collectionNameChanged(m_collectionName);
    }
}

QString CollectionDetailsModel::warningToString(DataTypeWarning::Warning warning) const
{
    return DataTypeWarning::getDataTypeWarningString(warning);
}


} // namespace QmlDesigner
