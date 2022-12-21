// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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

    if (m_import.isFileImport())
        return m_import.toString(true, true);

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

    if (m_import.isFileImport())
        return m_import.file();

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

bool ItemLibraryImport::updateCategoryVisibility(const QString &searchText, bool *changed)
{
    bool hasVisibleCategories = false;
    *changed = false;

    for (const auto &category : m_categoryModel.categorySections()) {
        bool categoryChanged = false;
        bool hasVisibleItems = category->updateItemVisibility(searchText, &categoryChanged);
        categoryChanged |= category->setVisible(hasVisibleItems);

        *changed |= categoryChanged;

        if (hasVisibleItems)
            hasVisibleCategories = true;

        if (searchText.isEmpty())
            category->setCategoryVisible(ItemLibraryModel::loadCategoryVisibleState(category->categoryName(),
                                                                                    importName()));
    }

    return hasVisibleCategories;
}

void ItemLibraryImport::showAllCategories()
{
    m_categoryModel.showAllCategories();
    setAllCategoriesVisible(true);
}

void ItemLibraryImport::hideCategory(const QString &categoryName)
{
    m_categoryModel.hideCategory(categoryName);
    setAllCategoriesVisible(false);
}

ItemLibraryCategory *ItemLibraryImport::selectCategory(int categoryIndex)
{
    return m_categoryModel.selectCategory(categoryIndex);
}

int ItemLibraryImport::selectFirstVisibleCategory()
{
    return m_categoryModel.selectFirstVisibleCategory();
}

void ItemLibraryImport::clearSelectedCategory(int categoryIndex)
{
    m_categoryModel.clearSelectedCategory(categoryIndex);
}

bool ItemLibraryImport::isAllCategoriesHidden() const
{
    if (!m_isVisible)
        return true;

    return m_categoryModel.isAllCategoriesHidden();
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

bool ItemLibraryImport::hasSingleCategory() const
{
    return m_categoryModel.rowCount() == 1;
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

ItemLibraryCategory *ItemLibraryImport::getCategoryByName(const QString &categoryName) const
{
    for (ItemLibraryCategory *catSec : std::as_const(m_categoryModel.categorySections())) {
        if (catSec->categoryName() == categoryName)
            return catSec;
    }

    return nullptr;
}

ItemLibraryCategory *ItemLibraryImport::getCategoryAt(int categoryIndex) const
{
    const QList<QPointer<ItemLibraryCategory>> categories = m_categoryModel.categorySections();

    if (categoryIndex != -1 && !categories.isEmpty())
        return categories.at(categoryIndex);

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

bool ItemLibraryImport::allCategoriesVisible() const
{
    return m_allCategoriesVisible;
}

void ItemLibraryImport::setAllCategoriesVisible(bool visible)
{
    m_allCategoriesVisible = visible;
}

} // namespace QmlDesigner
