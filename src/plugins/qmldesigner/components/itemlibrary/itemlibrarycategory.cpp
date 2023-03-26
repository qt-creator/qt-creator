// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "itemlibrarycategory.h"

#include "itemlibraryitem.h"
#include "itemlibrarywidget.h"

namespace QmlDesigner {

ItemLibraryCategory::ItemLibraryCategory(const QString &groupName, QObject *parent)
    : QObject(parent),
      m_ownerImport(qobject_cast<ItemLibraryImport *>(parent)),
      m_name(groupName)
{
}

QString ItemLibraryCategory::categoryName() const
{
    return m_name;
}

bool ItemLibraryCategory::categoryExpanded() const
{
    return m_categoryExpanded;
}

bool ItemLibraryCategory::categorySelected() const
{
    return m_categorySelected;
}

QString ItemLibraryCategory::sortingName() const
{
    if (ItemLibraryModel::categorySortingHash.contains(categoryName()))
        return ItemLibraryModel::categorySortingHash.value(categoryName());

    return categoryName();
}

void ItemLibraryCategory::addItem(ItemLibraryItem *itemEntry)
{
    m_itemModel.addItem(itemEntry);
}

QObject *ItemLibraryCategory::itemModel()
{
    return &m_itemModel;
}

bool ItemLibraryCategory::updateItemVisibility(const QString &searchText, bool *changed)
{
    bool hasVisibleItems = false;

    *changed = false;

    for (const auto &item : m_itemModel.items()) {
        bool itemVisible = item->itemName().toLower().contains(searchText)
                        || item->typeName().toLower().contains(searchText);

        if (searchText.isEmpty() && !item->isUsable())
            itemVisible = false;
        bool itemChanged = item->setVisible(itemVisible);

        *changed |= itemChanged;

        if (itemVisible)
            hasVisibleItems = true;
    }

    // update item model in horizontal view so search text matches item grid
    if (ItemLibraryWidget::isHorizontalLayout)
        m_itemModel.resetModel();

    // expand category if it has an item matching search criteria
    if (!searchText.isEmpty() && hasVisibleItems && !categoryExpanded())
        setExpanded(true);

    return hasVisibleItems;
}

void ItemLibraryCategory::setCategoryVisible(bool isVisible)
{
    if (isVisible != m_isVisible) {
        m_isVisible = isVisible;
        emit categoryVisibilityChanged();
    }
}

bool ItemLibraryCategory::setVisible(bool isVisible)
{
    if (isVisible != m_isVisible) {
        m_isVisible = isVisible;
        return true;
    }

    return false;
}

bool ItemLibraryCategory::isCategoryVisible() const
{
    return m_isVisible;
}

void ItemLibraryCategory::sortItems()
{
    m_itemModel.sortItems();
}

void ItemLibraryCategory::setExpanded(bool expanded)
{
    m_categoryExpanded = expanded;
}

void ItemLibraryCategory::setCategorySelected(bool selected)
{
    m_categorySelected = selected;
    emit categorySelectedChanged();
}

} // namespace QmlDesigner
