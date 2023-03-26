// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include <QStyledItemDelegate>

namespace QmlDesigner {

class NodeListDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    NodeListDelegate(QObject *parent = nullptr);
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;

protected:
    bool eventFilter(QObject *editor, QEvent *event) override;

private:
    void close();
    void commitAndClose();
};

} // namespace QmlDesigner.
