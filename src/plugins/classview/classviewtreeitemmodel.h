// Copyright (C) 2016 Denis Mingulov
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QStandardItemModel>

namespace ClassView {
namespace Internal {

class TreeItemModelPrivate;

class TreeItemModel : public QStandardItemModel
{
    Q_OBJECT

public:
    explicit TreeItemModel(QObject *parent = nullptr);
    ~TreeItemModel() override;

    void moveRootToTarget(const QStandardItem *target);

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool canFetchMore(const QModelIndex &parent) const override;
    void fetchMore(const QModelIndex &parent) override;
    bool hasChildren(const QModelIndex &parent = QModelIndex()) const override;

    Qt::DropActions supportedDragActions() const override;
    QStringList mimeTypes() const override;
    QMimeData *mimeData(const QModelIndexList &indexes) const override;

private:
    //! private class data pointer
    TreeItemModelPrivate *d;
};

} // namespace Internal
} // namespace ClassView
