// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include <QItemSelectionModel>

namespace QmlDesigner {

class NodeSelectionModel : public QItemSelectionModel
{
    Q_OBJECT

signals:
    void nodeSelected(const QStringList &events);

public:
    NodeSelectionModel(QAbstractItemModel *model);

    void select(const QItemSelection &selection, QItemSelectionModel::SelectionFlags command) override;

    QList<int> selectedNodes() const;
    void selectNode(int nodeId);

    void storeSelection();
    void reselect();

private:
    QItemSelection m_stored;
};

} // namespace QmlDesigner.
