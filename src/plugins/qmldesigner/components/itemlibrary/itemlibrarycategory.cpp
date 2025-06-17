// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "itemlibrarycategory.h"

#include "itemlibraryitem.h"
#include "itemlibrarytracing.h"
#include "itemlibrarywidget.h"

#include <QApplication>

namespace QmlDesigner {

using ItemLibraryTracing::category;

ItemLibraryCategory::ItemLibraryCategory(const QString &groupName, QObject *parent)
    : QObject(parent),
      m_ownerImport(qobject_cast<ItemLibraryImport *>(parent)),
      m_name(groupName)
{
    NanotraceHR::Tracer tracer{"item library category constructor", category()};
}

QString ItemLibraryCategory::categoryName() const
{
    NanotraceHR::Tracer tracer{"item library category name", category()};

    return m_name;
}

QString ItemLibraryCategory::displayNMame() const
{
    NanotraceHR::Tracer tracer{"item library category display name", category()};

    return QApplication::translate("itemlibrary", m_name.toUtf8());
}

bool ItemLibraryCategory::categoryExpanded() const
{
    NanotraceHR::Tracer tracer{"item library category expanded", category()};

    return m_categoryExpanded;
}

bool ItemLibraryCategory::categorySelected() const
{
    NanotraceHR::Tracer tracer{"item library category selected", category()};

    return m_categorySelected;
}

QString ItemLibraryCategory::sortingName() const
{
    NanotraceHR::Tracer tracer{"item library category sorting name", category()};

    if (ItemLibraryModel::categorySortingHash.contains(categoryName()))
        return ItemLibraryModel::categorySortingHash.value(categoryName());

    return categoryName();
}

void ItemLibraryCategory::addItem(ItemLibraryItem *itemEntry)
{
    NanotraceHR::Tracer tracer{"item library category add item", category()};

    m_itemModel.addItem(itemEntry);
}

QObject *ItemLibraryCategory::itemModel()
{
    return &m_itemModel;
}

bool ItemLibraryCategory::updateItemVisibility(const QString &searchText, bool *changed)
{
    NanotraceHR::Tracer tracer{"item library category update item visibility", category()};

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
    NanotraceHR::Tracer tracer{"item library category set visible", category()};

    if (isVisible != m_isVisible) {
        m_isVisible = isVisible;
        emit categoryVisibilityChanged();
    }
}

bool ItemLibraryCategory::setVisible(bool isVisible)
{
    NanotraceHR::Tracer tracer{"item library category set visible", category()};

    if (isVisible != m_isVisible) {
        m_isVisible = isVisible;
        return true;
    }

    return false;
}

bool ItemLibraryCategory::isCategoryVisible() const
{
    NanotraceHR::Tracer tracer{"item library category is visible", category()};

    return m_isVisible;
}

void ItemLibraryCategory::sortItems()
{
    NanotraceHR::Tracer tracer{"item library category sort items", category()};

    m_itemModel.sortItems();
}

void ItemLibraryCategory::setExpanded(bool expanded)
{
    NanotraceHR::Tracer tracer{"item library category set expanded", category()};

    m_categoryExpanded = expanded;
}

void ItemLibraryCategory::setCategorySelected(bool selected)
{
    NanotraceHR::Tracer tracer{"item library category set category selected", category()};

    m_categorySelected = selected;
    emit categorySelectedChanged();
}

} // namespace QmlDesigner
