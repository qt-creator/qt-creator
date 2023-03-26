// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "itemlibrarycategoriesmodel.h"
#include "import.h"

namespace QmlDesigner {

class ItemLibraryCategory;

class ItemLibraryImport : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString importName READ importName FINAL)
    Q_PROPERTY(QString importUrl READ importUrl FINAL)
    Q_PROPERTY(bool importVisible READ importVisible NOTIFY importVisibleChanged FINAL)
    Q_PROPERTY(bool importUsed READ importUsed NOTIFY importUsedChanged FINAL)
    Q_PROPERTY(bool importExpanded READ importExpanded WRITE setImportExpanded NOTIFY importExpandChanged FINAL)
    Q_PROPERTY(bool importRemovable READ importRemovable NOTIFY importRemovableChanged FINAL)
    Q_PROPERTY(bool importUnimported READ importUnimported FINAL)
    Q_PROPERTY(bool allCategoriesVisible READ allCategoriesVisible WRITE setAllCategoriesVisible NOTIFY allCategoriesVisibleChanged FINAL)
    Q_PROPERTY(QObject *categoryModel READ categoryModel NOTIFY categoryModelChanged FINAL)

public:
    enum class SectionType {
        Default,
        User,
        Quick3DAssets,
        Unimported
    };

    ItemLibraryImport(const Import &import, QObject *parent = nullptr,
                      SectionType sectionType = SectionType::Default);

    QString importName() const;
    QString importUrl() const;
    bool importExpanded() const;
    QString sortingName() const;
    Import importEntry() const;
    bool importVisible() const;
    bool importUsed() const;
    bool importRemovable() const;
    bool allCategoriesVisible() const;
    bool hasCategories() const;
    bool hasSingleCategory() const;
    bool isAllCategoriesHidden() const;
    ItemLibraryCategory *getCategoryByName(const QString &categoryName) const;
    ItemLibraryCategory *getCategoryAt(int categoryIndex) const;

    void addCategory(ItemLibraryCategory *category);
    QObject *categoryModel();
    bool updateCategoryVisibility(const QString &searchText, bool *changed);
    bool setVisible(bool isVisible);
    void setImportUsed(bool importUsed);
    void sortCategorySections();
    void setImportExpanded(bool expanded = true);
    void setAllCategoriesVisible(bool visible);
    void expandCategories(bool expand = true);
    void showAllCategories();
    void hideCategory(const QString &categoryName);
    ItemLibraryCategory *selectCategory(int categoryIndex);
    int selectFirstVisibleCategory();
    void clearSelectedCategory(int categoryIndex);
    bool importUnimported() const { return m_sectionType == SectionType::Unimported; }

    static QString userComponentsTitle();
    static QString quick3DAssetsTitle();
    static QString unimportedComponentsTitle();

    SectionType sectionType() const;

signals:
    void categoryModelChanged();
    void importVisibleChanged();
    void importUsedChanged();
    void importExpandChanged();
    void importRemovableChanged();
    void allCategoriesVisibleChanged();

private:
    void updateRemovable();

    Import m_import;
    bool m_importExpanded = true;
    bool m_isVisible = true;
    bool m_importUsed = false;
    bool m_importRemovable = false;
    bool m_allCategoriesVisible = true;
    SectionType m_sectionType = SectionType::Default;
    ItemLibraryCategoriesModel m_categoryModel;
};

} // namespace QmlDesigner
