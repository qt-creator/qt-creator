// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "listmodeleditormodel.h"

#include <abstractview.h>
#include <bindingproperty.h>
#include <nodelistproperty.h>
#include <nodeproperty.h>
#include <rewritertransaction.h>
#include <variantproperty.h>

#include <QVariant>

#include <algorithm>
#include <iterator>
#include <memory>
#include <ranges>

namespace QmlDesigner {

namespace {

class ListModelItem : public QStandardItem
{
public:
    ListModelItem(ModelNode node, PropertyName propertyName)
        : node(std::move(node))
        , propertyName(propertyName)
    {
        setEditable(true);
    }

    QVariant maybeConvertToNumber(const QVariant &value)
    {
        if (value.typeId() == QMetaType::Bool)
            return value;

        if (value.typeId() == QMetaType::QString) {
            const QString text = value.toString();
            if (text == "true")
                return QVariant(true);
            if (text == "false")
                return QVariant(false);
        }

        bool canConvert = false;
        double convertedValue = value.toDouble(&canConvert);
        if (canConvert) {
            return convertedValue;
        }

        return value;
    }

    QVariant data(int role) const override
    {
        if (role == Qt::BackgroundRole && hasInvalidValue)
            return QColor{Qt::darkYellow};

        return QStandardItem::data(role);
    }

    void setData(const QVariant &value, int role) override
    {
        if (role == Qt::DisplayRole || role == Qt::EditRole)
            hasInvalidValue = !value.isValid();

        if (role == Qt::EditRole) {
            const QVariant &convertedValue = maybeConvertToNumber(value);
            QStandardItem::setData(convertedValue, role);
            if (value.isValid())
                node.variantProperty(propertyName).setValue(convertedValue);
            else
                node.removeProperty(propertyName);
        } else {
            QStandardItem::setData(value, role);
        }
    }

    void removeProperty() { node.removeProperty(propertyName); }

    void renameProperty(const PropertyName &newPropertyName)
    {
        if (node.hasProperty(propertyName)) {
            node.removeProperty(propertyName);
            node.variantProperty(newPropertyName).setValue(data(Qt::EditRole));
        }
        propertyName = newPropertyName;
    }

public:
    ModelNode node;
    PropertyName propertyName;
    bool hasInvalidValue = false;
};

QList<PropertyName> getPropertyNames(const ModelNode &listElementNode)
{
    auto properties = listElementNode.variantProperties();

    QList<PropertyName> names;
    names.reserve(properties.size());

    for (const auto &property : properties)
        names.push_back(property.name().toByteArray());

    std::ranges::sort(names);

    return names;
}

QList<PropertyName> mergeProperyNames(const QList<PropertyName> &first,
                                      const QList<PropertyName> &second)
{
    QList<PropertyName> merged;
    merged.reserve(first.size() + second.size());

    std::ranges::set_union(first, second, std::back_inserter(merged));

    return merged;
}

std::unique_ptr<ListModelItem> createItem(const ModelNode &listElementNode,
                                          const PropertyName &propertyName)
{
    auto item = std::make_unique<ListModelItem>(listElementNode, propertyName);

    QVariant value = listElementNode.variantProperty(propertyName).value();

    item->setData(value, Qt::DisplayRole);

    return item;
}

QList<QString> convertToStringList(const QList<PropertyName> &propertyNames)
{
    QList<QString> names;
    names.reserve(propertyNames.size());

    for (const auto &propertyName : propertyNames)
        names.push_back(QString::fromUtf8(propertyName));

    return names;
}

QList<PropertyName> createProperyNames(const QList<ModelNode> &listElementNodes)
{
    QList<PropertyName> propertyNames;
    propertyNames.reserve(10);

    for (const ModelNode &listElementNode : listElementNodes)
        propertyNames = mergeProperyNames(getPropertyNames(listElementNode), propertyNames);

    return propertyNames;
}

QList<QStandardItem *> createColumnItems(const ModelNode &listModelNode,
                                         const PropertyName &propertyName)
{
    QList<QStandardItem *> items;
    const auto listElementNodes = listModelNode.defaultNodeListProperty().toModelNodeList();

    for (const ModelNode &listElementNode : listElementNodes)
        items.push_back(createItem(listElementNode, propertyName).release());

    return items;
}

void renameProperties(const QStandardItemModel *model,
                      int columnIndex,
                      const PropertyName &newPropertyName)
{
    for (int rowIndex = 0; rowIndex < model->rowCount(); ++rowIndex)
        static_cast<ListModelItem *>(model->item(rowIndex, columnIndex))->renameProperty(newPropertyName);
}

ModelNode listModelNode(const ModelNode &listViewNode,
                        const std::function<ModelNode()> &createModelCallback,
                        const std::function<ModelNode(const ModelNode &)> &goIntoComponentCallback)
{
    if (listViewNode.hasProperty("model")) {
        if (listViewNode.hasBindingProperty("model")) {
            return goIntoComponentCallback(listViewNode.bindingProperty("model").resolveToModelNode());
        } else if (listViewNode.hasNodeProperty("model")) {
            return goIntoComponentCallback(listViewNode.nodeProperty("model").modelNode());
        }
    }

    ModelNode newModel = createModelCallback();
    listViewNode.nodeProperty("model").reparentHere(newModel);

    return newModel;
}

} // namespace

void ListModelEditorModel::populateModel()
{
    const auto listElementNodes = m_listModelNode.defaultNodeListProperty().toModelNodeList();

    m_propertyNames = createProperyNames(listElementNodes);

    setHorizontalHeaderLabels(convertToStringList(m_propertyNames));

    createItems(listElementNodes);
}

void ListModelEditorModel::createItems(const QList<ModelNode> &listElementNodes)
{
    for (const ModelNode &listElementNode : listElementNodes)
        appendItems(listElementNode);
}

static QList<QStandardItem *> createRow(const auto &propertyNames, const auto &listElementNode)
{
    QList<QStandardItem *> row;
    row.reserve(propertyNames.size());
    for (const PropertyName &propertyName : propertyNames)
        row.push_back(createItem(listElementNode, propertyName).release());
    return row;
}

void ListModelEditorModel::insertItems(const ModelNode &listElementNode, int index)
{
    insertRow(index, createRow(m_propertyNames, listElementNode));
}

void ListModelEditorModel::appendItems(const ModelNode &listElementNode)
{
    appendRow(createRow(m_propertyNames, listElementNode));
}

void ListModelEditorModel::setListModel(ModelNode node)
{
    m_listModelNode = node;
    populateModel();
}

void ListModelEditorModel::setListView(ModelNode listView)
{
    setListModel(listModelNode(listView, m_createModelCallback, m_goIntoComponentCallback));
}

void ListModelEditorModel::addRow(int rowIndex)
{
    if (rowIndex < 0 || rowIndex > rowCount())
        return;

    NodeListProperty defaultNodeListProperty = m_listModelNode.defaultNodeListProperty();
    defaultNodeListProperty.view()->executeInTransaction(__FUNCTION__, [&] {
        auto newElement = m_createElementCallback();
        defaultNodeListProperty.reparentHere(newElement);
        defaultNodeListProperty.slide(defaultNodeListProperty.count() - 1, rowIndex);
        insertItems(newElement, rowIndex);
    });
}

void ListModelEditorModel::addColumn(const QString &columnName)
{
    PropertyName propertyName = columnName.toUtf8();

    auto found = std::ranges::lower_bound(m_propertyNames, propertyName);

    if (found != m_propertyNames.end() && *found == propertyName)
        return;

    int newIndex = static_cast<int>(std::distance(m_propertyNames.begin(), found));

    m_propertyNames.insert(found, propertyName);

    insertColumn(newIndex, createColumnItems(m_listModelNode, propertyName));

    setHorizontalHeaderItem(newIndex, new QStandardItem(columnName));
}

bool ListModelEditorModel::setValue(int row, int column, QVariant value, Qt::ItemDataRole role)
{
    QModelIndex index = createIndex(row, column, invisibleRootItem());
    bool success = setData(index, value, role);
    emit dataChanged(index, index);

    return success;
}

void ListModelEditorModel::removeColumn(int column)
{
    QList<QStandardItem *> columnItems = Super::takeColumn(column);
    m_propertyNames.removeAt(column);

    for (QStandardItem *columnItem : columnItems) {
        static_cast<ListModelItem *>(columnItem)->removeProperty();
        delete columnItem;
    }
}

void ListModelEditorModel::removeColumns(const QList<QModelIndex> &indices)
{
    using std::ranges::views::reverse;

    std::vector<int> columns = filterColumns(indices);

    for (int column : columns | reverse)
        removeColumn(column);
}

void ListModelEditorModel::removeRows(const QList<QModelIndex> &indices)
{
    using std::ranges::views::reverse;

    std::vector<int> rows = filterRows(indices);

    for (int row : rows | reverse)
        removeRow(row);
}

void ListModelEditorModel::removeRow(int row)
{
    QList<QStandardItem *> rowItems = Super::takeRow(row);

    if (rowItems.size())
        static_cast<ListModelItem *>(rowItems.front())->node.destroy();

    qDeleteAll(rowItems);
}

void ListModelEditorModel::renameColumn(int oldColumn, const QString &newColumnName)
{
    const PropertyName newPropertyName = newColumnName.toUtf8();

    auto found = std::ranges::lower_bound(m_propertyNames, newPropertyName);

    if (found != m_propertyNames.end() && *found == newPropertyName)
        return;

    int newColumn = static_cast<int>(std::distance(m_propertyNames.begin(), found));

    if (oldColumn == newColumn) {
        *found = newPropertyName;
        renameProperties(this, newColumn, newPropertyName);
    } else if (newColumn < oldColumn) {
        m_propertyNames.insert(found, newPropertyName);
        m_propertyNames.erase(std::next(m_propertyNames.begin(), oldColumn + 1));
        insertColumn(newColumn, takeColumn(oldColumn));
        renameProperties(this, newColumn, newPropertyName);
    } else {
        m_propertyNames.insert(found, newPropertyName);
        m_propertyNames.erase(std::next(m_propertyNames.begin(), oldColumn));
        insertColumn(newColumn - 1, takeColumn(oldColumn));
        renameProperties(this, newColumn - 1, newPropertyName);
    }

    setHorizontalHeaderLabels(convertToStringList(m_propertyNames));
}

QItemSelection ListModelEditorModel::moveRowsUp(const QList<QModelIndex> &indices)
{
    std::vector<int> rows = filterRows(indices);

    if (rows.empty() || rows.front() < 1)
        return {};

    auto nodeListProperty = m_listModelNode.defaultNodeListProperty();

    for (int row : rows) {
        insertRow(row - 1, takeRow(row));
        nodeListProperty.slide(row, row - 1);
    }

    return {index(rows.front() - 1, 0), index(rows.back() - 1, columnCount() - 1)};
}

QItemSelection ListModelEditorModel::moveRowsDown(const QList<QModelIndex> &indices)
{
    using std::ranges::views::reverse;

    std::vector<int> rows = filterRows(indices);

    if (rows.empty() || rows.back() >= (rowCount() - 1))
        return {};

    auto nodeListProperty = m_listModelNode.defaultNodeListProperty();

    for (int row : rows | reverse) {
        insertRow(row + 1, takeRow(row));
        nodeListProperty.slide(row, row + 1);
    }

    return {index(rows.back() + 1, 0), index(rows.front() + 1, columnCount() - 1)};
}

namespace {
void removeDuplicates(std::vector<int> &container)
{
    std::ranges::sort(container);

    auto removed = std::ranges::unique(container);
    container.erase(removed.begin(), removed.end());
}
} // namespace

std::vector<int> ListModelEditorModel::filterColumns(const QList<QModelIndex> &indices)
{
    std::vector<int> columns;
    columns.reserve(Utils::usize(indices));

    for (QModelIndex index : indices) {
        if (index.column() >= 0)
            columns.push_back(index.column());
    }

    removeDuplicates(columns);

    return columns;
}

std::vector<int> ListModelEditorModel::filterRows(const QList<QModelIndex> &indices)
{
    std::vector<int> rows;
    rows.reserve(Utils::usize(indices));

    for (QModelIndex index : indices) {
        if (index.row() >= 0)
            rows.push_back(index.row());
    }

    removeDuplicates(rows);

    return rows;
}

static int interactionRow(const QItemSelectionModel &selectionModel,
                          QModelIndex &(QModelIndexList::*defaultRowSelector)())
{
    QModelIndexList selectedRows = selectionModel.selectedRows();
    auto defaultRow = std::mem_fn(defaultRowSelector);
    int index = !selectedRows.isEmpty() ? defaultRow(selectedRows).row() : -1;
    if (index < 0 && selectionModel.hasSelection())
        index = defaultRow(selectionModel.selectedIndexes()).row();
    if (index < 0 && selectionModel.currentIndex().isValid())
        index = selectionModel.currentIndex().row();
    return index;
}

int ListModelEditorModel::currentInteractionRow(const QItemSelectionModel &selectionModel)
{
    return interactionRow(selectionModel, &QModelIndexList::first);
}

int ListModelEditorModel::nextInteractionRow(const QItemSelectionModel &selectionModel)
{
    return interactionRow(selectionModel, &QModelIndexList::last) + 1;
}

} // namespace QmlDesigner
