// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "itemlibrarymodel.h"

#include <QObject>
#include <QPointer>

namespace QmlDesigner {

class ItemLibraryCategory;

class ItemLibraryCategoriesModel : public QAbstractListModel
{
    Q_OBJECT

public:
    ItemLibraryCategoriesModel(QObject *parent = nullptr);
    ~ItemLibraryCategoriesModel() override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    QHash<int, QByteArray> roleNames() const override;

    void addCategory(ItemLibraryCategory *category);
    void expandCategories(bool expand = true);

    const QList<QPointer<ItemLibraryCategory>> &categorySections() const;

    bool isAllCategoriesHidden() const;
    void sortCategorySections();
    void resetModel();
    void showAllCategories();
    void hideCategory(const QString &categoryName);
    void clearSelectedCategory(int categoryIndex);
    int selectFirstVisibleCategory();
    QPointer<ItemLibraryCategory> selectCategory(int categoryIndex);

private:
    void addRoleNames();

    QList<QPointer<ItemLibraryCategory>> m_categoryList;
    QHash<int, QByteArray> m_roleNames;
};

} // namespace QmlDesigner
