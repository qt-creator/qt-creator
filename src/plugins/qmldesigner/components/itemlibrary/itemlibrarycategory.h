// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "itemlibraryitemsmodel.h"
#include "itemlibraryimport.h"

namespace QmlDesigner {

class ItemLibraryItem;

class ItemLibraryCategory : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString categoryName READ categoryName FINAL)
    Q_PROPERTY(bool categoryVisible READ isCategoryVisible WRITE setCategoryVisible NOTIFY categoryVisibilityChanged FINAL)
    Q_PROPERTY(bool categoryExpanded READ categoryExpanded WRITE setExpanded NOTIFY expandedChanged FINAL)
    Q_PROPERTY(bool categorySelected READ categorySelected WRITE setCategorySelected NOTIFY categorySelectedChanged FINAL)
    Q_PROPERTY(QObject *itemModel READ itemModel NOTIFY itemModelChanged FINAL)

public:
    ItemLibraryCategory(const QString &groupName, QObject *parent = nullptr);

    QString categoryName() const;
    bool categoryExpanded() const;
    bool categorySelected() const;
    QString sortingName() const;

    void addItem(ItemLibraryItem *item);
    QObject *itemModel();

    bool updateItemVisibility(const QString &searchText, bool *changed);

    void setCategoryVisible(bool isVisible);
    bool setVisible(bool isVisible);
    bool isCategoryVisible() const;

    void sortItems();

    void setExpanded(bool expanded);
    void setCategorySelected(bool selected);

    ItemLibraryImport *ownerImport() const { return m_ownerImport; }

signals:
    void itemModelChanged();
    void visibilityChanged();
    void expandedChanged();
    void categoryVisibilityChanged();
    void categorySelectedChanged();

private:
    ItemLibraryItemsModel m_itemModel;
    QPointer<ItemLibraryImport> m_ownerImport = nullptr;
    QString m_name;
    bool m_categoryExpanded = true;
    bool m_isVisible = true;
    bool m_categorySelected = false;
};

} // namespace QmlDesigner
