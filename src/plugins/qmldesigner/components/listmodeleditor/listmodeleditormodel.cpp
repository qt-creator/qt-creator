/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "listmodeleditormodel.h"

#include <abstractview.h>
#include <nodelistproperty.h>
#include <variantproperty.h>

#include <QVariant>

#include <algorithm>
#include <iterator>
#include <memory>

namespace QmlDesigner {

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
        bool canConvert = false;
        double convertedValue = value.toDouble(&canConvert);
        if (canConvert) {
            return convertedValue;
        }

        return value;
    }

    QVariant data(int role) const override
    {
        if (role == Qt::BackgroundColorRole && hasInvalidValue)
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

namespace {
QList<PropertyName> getPropertyNames(const ModelNode &listElementNode)
{
    auto properties = listElementNode.variantProperties();

    QList<PropertyName> names;
    names.reserve(properties.size());

    for (const auto &property : properties)
        names.push_back(property.name());

    std::sort(names.begin(), names.end());

    return names;
}

QList<PropertyName> mergeProperyNames(const QList<PropertyName> &first,
                                      const QList<PropertyName> &second)
{
    QList<PropertyName> merged;
    merged.reserve(first.size() + second.size());

    std::set_union(first.begin(),
                   first.end(),
                   second.begin(),
                   second.end(),
                   std::back_inserter(merged));

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

void ListModelEditorModel::appendItems(const ModelNode &listElementNode)
{
    QList<QStandardItem *> row;
    row.reserve(m_propertyNames.size());
    for (const PropertyName &propertyName : propertyNames())
        row.push_back(createItem(listElementNode, propertyName).release());

    appendRow(row);
}

void ListModelEditorModel::addRow()
{
    auto newElement = m_listModelNode.view()->createModelNode("QtQml.Models.ListElement", 2, 15);
    m_listModelNode.defaultNodeListProperty().reparentHere(newElement);

    appendItems(newElement);
}

void ListModelEditorModel::addColumn(const QString &columnName)
{
    PropertyName propertyName = columnName.toUtf8();

    auto found = std::lower_bound(m_propertyNames.begin(), m_propertyNames.end(), propertyName);

    if (found != m_propertyNames.end() && *found == columnName)
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
    QList<QStandardItem *> columnItems = QStandardItemModel::takeColumn(column);
    m_propertyNames.removeAt(column);

    for (QStandardItem *columnItem : columnItems) {
        static_cast<ListModelItem *>(columnItem)->removeProperty();
        delete columnItem;
    }
}

void ListModelEditorModel::removeRow(int row)
{
    QList<QStandardItem *> rowItems = QStandardItemModel::takeRow(row);

    if (rowItems.size())
        static_cast<ListModelItem *>(rowItems.front())->node.destroy();

    qDeleteAll(rowItems);
}

void ListModelEditorModel::renameColumn(int oldColumn, const QString &newColumnName)
{
    const PropertyName newPropertyName = newColumnName.toUtf8();

    auto found = std::lower_bound(m_propertyNames.begin(), m_propertyNames.end(), newPropertyName);

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

} // namespace QmlDesigner
