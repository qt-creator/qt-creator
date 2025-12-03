// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "itemlibraryimport.h"
#include "itemlibrarycategory.h"
#include "itemlibrarytracing.h"

namespace QmlDesigner {

using ItemLibraryTracing::category;

ItemLibraryImport::ItemLibraryImport(const Import &import, QObject *parent, SectionType sectionType)
    : QObject(parent),
      m_import(import),
      m_sectionType(sectionType)
{
    NanotraceHR::Tracer tracer{"item library import constructor", category()};

    updateRemovable();
}

QString ItemLibraryImport::importName() const
{
    NanotraceHR::Tracer tracer{"item library import name", category()};

    if (m_sectionType == SectionType::User)
        return userComponentsTitle();

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
    NanotraceHR::Tracer tracer{"item library import URL", category()};

    if (m_sectionType == SectionType::User)
        return userComponentsTitle();

    if (m_sectionType == SectionType::Unimported)
        return unimportedComponentsTitle();

    if (m_import.isFileImport())
        return m_import.file();

    return m_import.url();
}

bool ItemLibraryImport::importExpanded() const
{
    NanotraceHR::Tracer tracer{"item library import expanded", category()};

    return m_importExpanded;
}

QString ItemLibraryImport::sortingName() const
{
    NanotraceHR::Tracer tracer{"item library import sorting name", category()};

    if (m_sectionType == SectionType::User)
        return "_"; // user components always come first

    if (m_sectionType == SectionType::Unimported)
        return "zzzzzz"; // Unimported components come last

    if (!hasCategories()) // imports with no categories come before last
        return "zzzzz_" + importName();

    return importName();
}

void ItemLibraryImport::addCategory(ItemLibraryCategory *category)
{
    NanotraceHR::Tracer tracer{"item library import add category", ItemLibraryTracing::category()};

    m_categoryModel.addCategory(category);
}

QObject *ItemLibraryImport::categoryModel()
{
    NanotraceHR::Tracer tracer{"item library import category model", category()};

    return &m_categoryModel;
}

void ItemLibraryImport::expandCategories(bool expand)
{
    NanotraceHR::Tracer tracer{"item library import expand categories", category()};

    m_categoryModel.expandCategories(expand);
}

bool ItemLibraryImport::updateCategoryVisibility(const QString &searchText, bool *changed)
{
    NanotraceHR::Tracer tracer{"item library import update category visibility", category()};

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
    NanotraceHR::Tracer tracer{"item library import show all categories", category()};

    m_categoryModel.showAllCategories();
    setAllCategoriesVisible(true);
}

void ItemLibraryImport::hideCategory(const QString &categoryName)
{
    NanotraceHR::Tracer tracer{"item library import hide category", category()};

    m_categoryModel.hideCategory(categoryName);
    setAllCategoriesVisible(false);
}

ItemLibraryCategory *ItemLibraryImport::selectCategory(int categoryIndex)
{
    NanotraceHR::Tracer tracer{"item library import select category", category()};

    return m_categoryModel.selectCategory(categoryIndex);
}

int ItemLibraryImport::selectFirstVisibleCategory()
{
    NanotraceHR::Tracer tracer{"item library import select first visible category", category()};

    return m_categoryModel.selectFirstVisibleCategory();
}

void ItemLibraryImport::clearSelectedCategory(int categoryIndex)
{
    NanotraceHR::Tracer tracer{"item library import clear selected category", category()};

    m_categoryModel.clearSelectedCategory(categoryIndex);
}

bool ItemLibraryImport::isAllCategoriesHidden() const
{
    NanotraceHR::Tracer tracer{"item library import is all categories hidden", category()};

    if (!m_isVisible)
        return true;

    return m_categoryModel.isAllCategoriesHidden();
}

Import ItemLibraryImport::importEntry() const
{
    NanotraceHR::Tracer tracer{"item library import entry", category()};

    return m_import;
}

bool ItemLibraryImport::setVisible(bool isVisible)
{
    NanotraceHR::Tracer tracer{"item library import set visible", category()};

    if (isVisible != m_isVisible) {
        m_isVisible = isVisible;
        emit importVisibleChanged();
        return true;
    }

    return false;
}

bool ItemLibraryImport::importVisible() const
{
    NanotraceHR::Tracer tracer{"item library import is visible", category()};

    return m_isVisible;
}

void ItemLibraryImport::setImportUsed(bool importUsed)
{
    NanotraceHR::Tracer tracer{"item library import set used", category()};

    if (importUsed != m_importUsed) {
        m_importUsed = importUsed;
        updateRemovable();
        emit importUsedChanged();
    }
}

bool ItemLibraryImport::importUsed() const
{
    NanotraceHR::Tracer tracer{"item library import is used", category()};

    return m_importUsed;
}

bool ItemLibraryImport::importRemovable() const
{
    NanotraceHR::Tracer tracer{"item library import is removable", category()};

    return m_importRemovable;
}

bool ItemLibraryImport::hasCategories() const
{
    NanotraceHR::Tracer tracer{"item library import has categories", category()};

    return m_categoryModel.rowCount() > 0;
}

bool ItemLibraryImport::hasSingleCategory() const
{
    NanotraceHR::Tracer tracer{"item library import has single category", category()};

    return m_categoryModel.rowCount() == 1;
}

void ItemLibraryImport::sortCategorySections()
{
    NanotraceHR::Tracer tracer{"item library import sort category sections", category()};

    m_categoryModel.sortCategorySections();
}

void ItemLibraryImport::setImportExpanded(bool expanded)
{
    NanotraceHR::Tracer tracer{"item library import set expanded", category()};

    if (expanded != m_importExpanded) {
        m_importExpanded = expanded;
        emit importExpandChanged();
    }
}

ItemLibraryCategory *ItemLibraryImport::getCategoryByName(const QString &categoryName) const
{
    NanotraceHR::Tracer tracer{"item library import get category by name", category()};

    for (ItemLibraryCategory *catSec : std::as_const(m_categoryModel.categorySections())) {
        if (catSec->categoryName() == categoryName)
            return catSec;
    }

    return nullptr;
}

ItemLibraryCategory *ItemLibraryImport::getCategoryAt(int categoryIndex) const
{
    NanotraceHR::Tracer tracer{"item library import get category at index", category()};

    const QList<QPointer<ItemLibraryCategory>> categories = m_categoryModel.categorySections();

    if (categoryIndex != -1 && !categories.isEmpty())
        return categories.at(categoryIndex);

    return nullptr;
}

// static
QString ItemLibraryImport::userComponentsTitle()
{
    NanotraceHR::Tracer tracer{"item library import user components title", category()};

    return tr("My Components");
}

// static
QString ItemLibraryImport::unimportedComponentsTitle()
{
    NanotraceHR::Tracer tracer{"item library import unimported components title", category()};

    return tr("All Other Components");
}

ItemLibraryImport::SectionType ItemLibraryImport::sectionType() const
{
    NanotraceHR::Tracer tracer{"item library import section type", category()};

    return m_sectionType;
}

void ItemLibraryImport::updateRemovable()
{
    NanotraceHR::Tracer tracer{"item library import update removable", category()};

#ifdef QDS_USE_PROJECTSTORAGE
    bool importRemovable = m_sectionType == SectionType::Default && m_import.url() != "QtQuick";
#else
    bool importRemovable = !m_importUsed && m_sectionType == SectionType::Default
                           && m_import.url() != "QtQuick";
#endif
    if (importRemovable != m_importRemovable) {
        m_importRemovable = importRemovable;
        emit importRemovableChanged();
    }
}

bool ItemLibraryImport::allCategoriesVisible() const
{
    NanotraceHR::Tracer tracer{"item library import all categories visible", category()};

    return m_allCategoriesVisible;
}

void ItemLibraryImport::setAllCategoriesVisible(bool visible)
{
    NanotraceHR::Tracer tracer{"item library import set all categories visible", category()};

    m_allCategoriesVisible = visible;
}

} // namespace QmlDesigner
