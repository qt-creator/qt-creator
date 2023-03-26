// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QAbstractItemModel>
#include <QFileInfo>
#include <QList>
#include <QSet>
#include <QStringList>

namespace QmlDesigner {
namespace Internal {

class AssetImportUpdateTreeItem;

class AssetImportUpdateTreeModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    AssetImportUpdateTreeModel(QObject *parent = nullptr);
    ~AssetImportUpdateTreeModel() override;

    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

    void createItems(const QList<QFileInfo> &infos, const QSet<QString> &preselectedFiles);
    QStringList checkedFiles() const;

    static AssetImportUpdateTreeItem *treeItemAtIndex(const QModelIndex &idx);

public slots:
    void clear();

private:
    QModelIndex index(AssetImportUpdateTreeItem *item) const;
    QVariant data(const AssetImportUpdateTreeItem *row, int role) const;
    bool setCheckState(const QModelIndex &idx, Qt::CheckState checkState, bool firstCall = true);

    AssetImportUpdateTreeItem *m_rootItem;
    QList<AssetImportUpdateTreeItem *> m_fileItems;
};

} // namespace Internal
} // namespace QmlDesigner
