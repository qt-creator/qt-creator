// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QAbstractListModel>
#include <QObject>
#include <QPointer>

namespace QmlDesigner {

class ItemLibraryItem;

class ItemLibraryItemsModel : public QAbstractListModel
{
    Q_OBJECT

public:
    ItemLibraryItemsModel(QObject *parent = nullptr);
    ~ItemLibraryItemsModel() override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void addItem(ItemLibraryItem *item);

    const QList<QPointer<ItemLibraryItem>> &items() const;

    void sortItems();
    void resetModel();

private:
    void addRoleNames();

    QList<QPointer<ItemLibraryItem>> m_itemList;
    QHash<int, QByteArray> m_roleNames;
};

} // namespace QmlDesigner
