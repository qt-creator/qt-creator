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

ItemLibraryImport::ItemLibraryImport(const Import &import, QObject *parent, SectionType sectionType)
    : QObject(parent),
      m_import(import),
      m_sectionType(sectionType)
{
    updateRemovable();
}

QString ItemLibraryImport::importName() const
{
    if (m_sectionType == SectionType::User)
        return userComponentsTitle();

    if (m_sectionType == SectionType::Quick3DAssets)
        return quick3DAssetsTitle();

    if (m_sectionType == SectionType::Unimported)
        return unimportedComponentsTitle();

    if (importUrl() == "QtQuick")
        return tr("Default Components");

    return importUrl().replace('.', ' ');
}

QString ItemLibraryImport::importUrl() const
{
    if (m_sectionType == SectionType::User)
        return userComponentsTitle();

    if (m_sectionType == SectionType::Quick3DAssets)
        return quick3DAssetsTitle();

    if (m_sectionType == SectionType::Unimported)
        return unimportedComponentsTitle();

    return m_import.url();
}

bool ItemLibraryImport::importExpanded() const
{
    return m_importExpanded;
}

QString ItemLibraryImport::sortingName() const
{
    if (m_sectionType == SectionType::User)
        return "_"; // user components always come first

    if (m_sectionType == SectionType::Quick3DAssets)
        return "__";  // Quick3DAssets come second

    if (m_sectionType == SectionType::Unimported)
        return "zzzzzz"; // Unimported components come last

    if (!hasCategories()) // imports with no categories come before last
        return "zzzzz_" + importName();

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

void ItemLibraryImport::expandCategories(bool expand)
{
    m_categoryModel.expandCategories(expand);
}

bool ItemLibraryImport::updateCategoryVisibility(const QString &searchText, bool *changed, bool expand)
{
    bool hasVisibleCategories = false;
    *changed = false;

    for (const auto &category : m_categoryModel.categorySections()) {
        bool categoryChanged = false;
        bool hasVisibleItems = category->updateItemVisibility(searchText, &categoryChanged, expand);
        categoryChanged |= category->setVisible(hasVisibleItems);

        *changed |= categoryChanged;

        if (hasVisibleItems)
            hasVisibleCategories = true;
    }

    return hasVisibleCategories;
}

Import ItemLibraryImport::importEntry() const
{
    return m_import;
}

bool ItemLibraryImport::setVisible(bool isVisible)
{
    if (isVisible != m_isVisible) {
        m_isVisible = isVisible;
        emit importVisibleChanged();
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
    if (importUsed != m_importUsed) {
        m_importUsed = importUsed;
        updateRemovable();
        emit importUsedChanged();
    }
}

bool ItemLibraryImport::importUsed() const
{
    return m_importUsed;
}

bool ItemLibraryImport::importRemovable() const
{
    return m_importRemovable;
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
    if (expanded != m_importExpanded) {
        m_importExpanded = expanded;
        emit importExpandChanged();
    }
}

ItemLibraryCategory *ItemLibraryImport::getCategorySection(const QString &categoryName) const
{
    for (ItemLibraryCategory *catSec : std::as_const(m_categoryModel.categorySections())) {
        if (catSec->categoryName() == categoryName)
            return catSec;
    }

    return nullptr;
}

// static
QString ItemLibraryImport::userComponentsTitle()
{
    return tr("My Components");
}

// static
QString ItemLibraryImport::quick3DAssetsTitle()
{
    return tr("My 3D Components");
}

// static
QString ItemLibraryImport::unimportedComponentsTitle()
{
    return tr("All Other Components");
}

ItemLibraryImport::SectionType ItemLibraryImport::sectionType() const
{
    return m_sectionType;
}

void ItemLibraryImport::updateRemovable()
{
    bool importRemovable = !m_importUsed && m_sectionType == SectionType::Default
            && m_import.url() != "QtQuick";
    if (importRemovable != m_importRemovable) {
        m_importRemovable = importRemovable;
        emit importRemovableChanged();
    }
}

} // namespace QmlDesigner
