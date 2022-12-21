// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <modelnode.h>

#include <QItemSelection>
#include <QStandardItemModel>

namespace QmlDesigner {

class ListModelEditorModel : public QStandardItemModel
{
    using QStandardItemModel::removeColumns;
    using QStandardItemModel::removeRows;

public:
    ListModelEditorModel(std::function<ModelNode()> createModelCallback,
                         std::function<ModelNode()> createElementCallback,
                         std::function<ModelNode(const ModelNode &)> goIntoComponentCallback)
        : m_createModelCallback(std::move(createModelCallback))
        , m_createElementCallback(std::move(createElementCallback))
        , m_goIntoComponentCallback(std::move(goIntoComponentCallback))
    {}

    void setListModel(ModelNode node);

    void setListView(ModelNode listView);

    void addRow();
    void addColumn(const QString &columnName);

    const QList<QmlDesigner::PropertyName> &propertyNames() const { return m_propertyNames; }

    bool setValue(int row, int column, QVariant value, Qt::ItemDataRole role = Qt::EditRole);

    void removeColumns(const QList<QModelIndex> &indices);
    void removeRows(const QList<QModelIndex> &indices);
    void renameColumn(int column, const QString &newColumnName);
    QItemSelection moveRowsUp(const QList<QModelIndex> &indices);
    QItemSelection moveRowsDown(const QList<QModelIndex> &indices);

    static std::vector<int> filterColumns(const QList<QModelIndex> &indices);
    static std::vector<int> filterRows(const QList<QModelIndex> &indices);

private:
    void removeRow(int row);
    void removeColumn(int column);
    void populateModel();
    void createItems(const QList<ModelNode> &listElementNodes);
    void appendItems(const ModelNode &listElementNode);

private:
    ModelNode m_listModelNode;
    QList<QmlDesigner::PropertyName> m_propertyNames;
    std::function<ModelNode()> m_createModelCallback;
    std::function<ModelNode()> m_createElementCallback;
    std::function<ModelNode(const ModelNode &)> m_goIntoComponentCallback;
};

} // namespace QmlDesigner
