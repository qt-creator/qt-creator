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

#include <QStandardItemModel>

namespace QmlDesigner {

class ListModelEditorModel : public QStandardItemModel
{

public:
    void setListModel(ModelNode node)
    {
        m_listModelNode = node;
        populateModel();
    }

    void addRow();
    void addColumn(const QString &columnName);

    const QList<QmlDesigner::PropertyName> &propertyNames() const { return m_propertyNames; }

    bool setValue(int row, int column, QVariant value, Qt::ItemDataRole role = Qt::EditRole);

    void removeColumn(int column);
    void removeRow(int row);
    void renameColumn(int column, const QString &newColumnName);

private:
    void populateModel();
    void createItems(const QList<ModelNode> &listElementNodes);
    void appendItems(const ModelNode &listElementNode);

private:
    ModelNode m_listModelNode;
    QList<QmlDesigner::PropertyName> m_propertyNames;
};

} // namespace QmlDesigner
