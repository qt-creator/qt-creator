// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "collectiondetailsmodel.h"

#include "collectiondatatypemodel.h"
#include "collectioneditorutils.h"
#include "modelnode.h"

#include <coreplugin/editormanager/editormanager.h>

#include <utils/fileutils.h>
#include <utils/qtcassert.h>
#include <utils/textfileformat.h>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>

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

    QTC_ASSERT(m_currentCollection.hasValidReference(), return {});

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
    QTC_ASSERT(m_currentCollection.hasValidReference(), return false);

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
    QTC_ASSERT(m_currentCollection.hasValidReference(), return false);

    if (orientation == Qt::Vertical)
        return false;

    bool headerChanged = m_currentCollection.setPropertyName(section, value.toString());
    if (headerChanged)
        emit this->headerDataChanged(orientation, section, section);

    return headerChanged;
}

bool CollectionDetailsModel::insertRows(int row, int count, [[maybe_unused]] const QModelIndex &parent)
{
    QTC_ASSERT(m_currentCollection.hasValidReference(), return false);

    if (count < 1)
        return false;

    row = qBound(0, row, rowCount());

    beginResetModel();
    m_currentCollection.insertEmptyRows(row, count);
    endResetModel();

    selectRow(row);
    return true;
}

bool CollectionDetailsModel::removeColumns(int column, int count, const QModelIndex &parent)
{
    QTC_ASSERT(m_currentCollection.hasValidReference(), return false);

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
    QTC_ASSERT(m_currentCollection.hasValidReference(), return false);

    if (row < 0 || row >= rowCount(parent) || count < 1)
        return false;

    count = std::min(count, rowCount(parent) - row);
    beginRemoveRows(parent, row, row + count - 1);
    bool rowsRemoved = m_currentCollection.removeRows(row, count);
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
            return CollectionDataTypeModel::dataTypeToString(m_currentCollection.typeAt(section));
        else
            return m_currentCollection.propertyAt(section);
    }

    if (orientation == Qt::Vertical)
        return section + 1;

    return {};
}

CollectionDetails::DataType CollectionDetailsModel::propertyDataType(int column) const
{
    QTC_ASSERT(m_currentCollection.hasValidReference(), return CollectionDetails::DataType::Unknown);

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
    QTC_ASSERT(m_currentCollection.hasValidReference(), return {});

    return m_currentCollection.propertyAt(column);
}

QString CollectionDetailsModel::propertyType(int column) const
{
    QTC_ASSERT(m_currentCollection.hasValidReference(), return {});

    return CollectionDataTypeModel::dataTypeToString(m_currentCollection.typeAt(column));
}

bool CollectionDetailsModel::isPropertyAvailable(const QString &name)
{
    QTC_ASSERT(m_currentCollection.hasValidReference(), return false);

    return m_currentCollection.containsPropertyName(name);
}

bool CollectionDetailsModel::addColumn(int column, const QString &name, const QString &propertyType)
{
    QTC_ASSERT(m_currentCollection.hasValidReference(), return false);

    if (m_currentCollection.containsPropertyName(name))
        return false;

    if (column < 0 || column > columnCount())
        column = columnCount();

    beginInsertColumns({}, column, column);
    m_currentCollection.insertColumn(name,
                                     column,
                                     {},
                                     CollectionDataTypeModel::dataTypeFromString(propertyType));
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
    QTC_ASSERT(m_currentCollection.hasValidReference(), return false);

    bool changed = m_currentCollection.setPropertyType(column,
                                                       CollectionDataTypeModel::dataTypeFromString(
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
        loadJsonCollection(fileName, collection);
    }
}

void CollectionDetailsModel::removeCollection(const ModelNode &sourceNode, const QString &collection)
{
    CollectionReference collectionRef{sourceNode, collection};
    if (!m_openedCollections.contains(collectionRef))
        return;

    if (m_currentCollection.reference() == collectionRef)
        loadCollection({}, {});

    m_openedCollections.remove(collectionRef);
}

void CollectionDetailsModel::removeAllCollections()
{
    loadCollection({}, {});
    m_openedCollections.clear();
}

void CollectionDetailsModel::renameCollection(const ModelNode &sourceNode,
                                              const QString &oldName,
                                              const QString &newName)
{
    CollectionReference oldRef{sourceNode, oldName};
    if (!m_openedCollections.contains(oldRef))
        return;

    CollectionReference newReference{sourceNode, newName};
    bool collectionIsSelected = m_currentCollection.reference() == oldRef;
    CollectionDetails collection = m_openedCollections.take(oldRef);
    collection.resetReference(newReference);
    m_openedCollections.insert(newReference, collection);

    if (collectionIsSelected)
        setCollectionName(newName);
}

bool CollectionDetailsModel::saveDataStoreCollections()
{
    const ModelNode node = m_currentCollection.reference().node;
    const Utils::FilePath path = CollectionEditorUtils::dataStoreJsonFilePath();
    Utils::FileReader fileData;

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
                obj.insert(reference.name, openedCollection.toLocalJson());
                collectionsToBeSaved << openedCollection;
            }
        }

        document.setObject(obj);

        if (CollectionEditorUtils::writeToJsonDocument(path, document)) {
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

    QTC_ASSERT(m_currentCollection.hasValidReference(), return false);

    bool saved = false;
    const FilePath filePath = FilePath::fromUserInput(url.isLocalFile() ? url.toLocalFile()
                                                                        : url.toString());
    const QString saveFormat = filePath.toFileInfo().suffix().toLower();
    const QString content = saveFormat == "csv" ? m_currentCollection.toCsv()
                                                : m_currentCollection.toJson();

    TextFileFormat textFileFormat;
    textFileFormat.codec = EditorManager::defaultTextCodec();
    textFileFormat.lineTerminationMode = EditorManager::defaultLineEnding();
    QString errorMessage;
    saved = textFileFormat.writeFile(filePath, content, &errorMessage);

    if (!saved)
        qWarning() << Q_FUNC_INFO << "Unable to write file" << errorMessage;

    return saved;
}

const CollectionDetails CollectionDetailsModel::upToDateConstCollection(
    const CollectionReference &reference) const
{
    using Utils::FilePath;
    using Utils::FileReader;
    CollectionDetails collection;

    if (m_openedCollections.contains(reference)) {
        collection = m_openedCollections.value(reference);
    } else {
        QUrl url = CollectionEditorUtils::getSourceCollectionPath(reference.node);
        FilePath path = FilePath::fromUserInput(url.isLocalFile() ? url.toLocalFile()
                                                                  : url.toString());
        FileReader file;

        if (!file.fetch(path))
            return collection;

        QJsonParseError jpe;
        QJsonDocument document = QJsonDocument::fromJson(file.data(), &jpe);

        if (jpe.error != QJsonParseError::NoError)
            return collection;

        collection = CollectionDetails::fromLocalJson(document, reference.name);
        collection.resetReference(reference);
    }
    return collection;
}

bool CollectionDetailsModel::collectionHasColumn(const CollectionReference &reference,
                                                 const QString &columnName) const
{
    const CollectionDetails collection = upToDateConstCollection(reference);
    return collection.containsPropertyName(columnName);
}

QString CollectionDetailsModel::getFirstColumnName(const CollectionReference &reference) const
{
    const CollectionDetails collection = upToDateConstCollection(reference);
    return collection.propertyAt(0);
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
    if (m_currentCollection.hasValidReference()) {
        closeCollectionIfSaved(m_currentCollection.reference());
        m_currentCollection = CollectionDetails{};
    }
}

void CollectionDetailsModel::loadJsonCollection(const QString &filePath, const QString &collection)
{
    QJsonDocument document = readJsonFile(filePath);

    beginResetModel();
    m_currentCollection.resetData(document, collection);
    ensureSingleCell();
    endResetModel();
}

void CollectionDetailsModel::ensureSingleCell()
{
    if (!m_currentCollection.hasValidReference())
        return;

    if (!columnCount())
        addColumn(0, "Column 1", "String");

    if (!rowCount())
        insertRow(0);

    updateEmpty();
}

QJsonDocument CollectionDetailsModel::readJsonFile(const QUrl &url)
{
    using Utils::FilePath;
    using Utils::FileReader;
    FilePath path = FilePath::fromUserInput(url.isLocalFile() ? url.toLocalFile() : url.toString());
    FileReader file;

    if (!file.fetch(path)) {
        emit warning(tr("File reading problem"), file.errorString());
        return {};
    }

    QJsonParseError jpe;
    QJsonDocument document = QJsonDocument::fromJson(file.data(), &jpe);

    if (jpe.error != QJsonParseError::NoError)
        emit warning(tr("Json parse error"), jpe.errorString());

    return document;
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
