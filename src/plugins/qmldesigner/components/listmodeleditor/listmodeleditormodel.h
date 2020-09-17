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
