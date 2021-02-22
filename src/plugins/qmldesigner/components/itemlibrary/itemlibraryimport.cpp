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

#include "itemlibraryimport.h"
#include "itemlibrarycategory.h"

namespace QmlDesigner {

ItemLibraryImport::ItemLibraryImport(const Import &import, QObject *parent, bool isUserSection)
    : QObject(parent),
      m_import(import),
      m_isUserSection(isUserSection)
{
}

QString ItemLibraryImport::importName() const
{
    if (m_isUserSection)
        return userComponentsTitle();

    if (importUrl() == "QtQuick")
        return tr("Default Components");

    return importUrl().replace('.', ' ');
}

QString ItemLibraryImport::importUrl() const
{
    if (m_isUserSection)
        return userComponentsTitle();

    return m_import.url();
}

bool ItemLibraryImport::importExpanded() const
{
    return m_importExpanded;
}

QString ItemLibraryImport::sortingName() const
{
    if (m_isUserSection) // user components always come first
        return "_";

    if (!hasCategories()) // imports with no categories are at the bottom of the list
        return "zzzzz" + importName();

    return importName();
}

void ItemLibraryImport::addCategory(ItemLibraryCategory *category)
{
    m_categoryModel.addCategory(category);
}

QObject *ItemLibraryImport::categoryModel()
{
    return &m_categoryModel;
}

bool ItemLibraryImport::updateCategoryVisibility(const QString &searchText, bool *changed)
{
    bool hasVisibleItems = false;

    *changed = false;

    for (const auto &category : m_categoryModel.categorySections()) {
        bool categoryChanged = false;
        hasVisibleItems = category->updateItemVisibility(searchText, &categoryChanged);
        categoryChanged |= category->setVisible(hasVisibleItems);

        *changed |= categoryChanged;
        *changed |= hasVisibleItems;
    }

    if (*changed)
        m_categoryModel.resetModel();

    return hasVisibleItems;
}

Import ItemLibraryImport::importEntry() const
{
    return m_import;
}

bool ItemLibraryImport::setVisible(bool isVisible)
{
    if (isVisible != m_isVisible) {
        m_isVisible = isVisible;
        return true;
    }

    return false;
}

bool ItemLibraryImport::importVisible() const
{
    return m_isVisible;
}

void ItemLibraryImport::setImportUsed(bool importUsed)
{
    m_importUsed = importUsed;
}

bool ItemLibraryImport::importUsed() const
{
    return m_importUsed;
}

bool ItemLibraryImport::hasCategories() const
{
    return m_categoryModel.rowCount() > 0;
}

void ItemLibraryImport::sortCategorySections()
{
    m_categoryModel.sortCategorySections();
}

void ItemLibraryImport::setImportExpanded(bool expanded)
{
    m_importExpanded = expanded;
}

ItemLibraryCategory *ItemLibraryImport::getCategorySection(const QString &categoryName) const
{
    for (ItemLibraryCategory *catSec : std::as_const(m_categoryModel.categorySections())) {
        if (catSec->categoryName() == categoryName)
            return catSec;
    }

    return nullptr;
}

bool ItemLibraryImport::isUserSection() const
{
    return m_isUserSection;
}

// static
QString ItemLibraryImport::userComponentsTitle()
{
    return tr("My Components");
}

} // namespace QmlDesigner
