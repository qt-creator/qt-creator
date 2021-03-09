/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include "itemlibrarycategory.h"

#include "itemlibraryitem.h"

namespace QmlDesigner {

ItemLibraryCategory::ItemLibraryCategory(const QString &groupName, QObject *parent)
    : QObject(parent),
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

QString ItemLibraryCategory::sortingName() const
{
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

bool ItemLibraryCategory::updateItemVisibility(const QString &searchText, bool *changed, bool expand)
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

    // expand category if it has an item matching search criteria
    if (expand && hasVisibleItems && !categoryExpanded())
        setExpanded(true);

    return hasVisibleItems;
}

bool ItemLibraryCategory::setVisible(bool isVisible)
{
    if (isVisible != m_isVisible) {
        m_isVisible = isVisible;
        return true;
    }

    return false;
}

bool ItemLibraryCategory::isVisible() const
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

} // namespace QmlDesigner
