// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "itemlibrarycategoriesmodel.h"
#include "itemlibrarycategory.h"
#include "itemlibrarymodel.h"

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QDebug>
#include <QVariant>
#include <QMetaProperty>
#include <QMetaObject>

namespace QmlDesigner {

ItemLibraryCategoriesModel::ItemLibraryCategoriesModel(QObject *parent) :
    QAbstractListModel(parent)
{
    addRoleNames();
}

ItemLibraryCategoriesModel::~ItemLibraryCategoriesModel()
{
}

int ItemLibraryCategoriesModel::rowCount(const QModelIndex &) const
{
    return m_categoryList.size();
}

QVariant ItemLibraryCategoriesModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_categoryList.size()) {
        qWarning() << Q_FUNC_INFO << "invalid index requested";
        return {};
    }

    if (m_roleNames.contains(role)) {
        QVariant value = m_categoryList.at(index.row())->property(m_roleNames.value(role));

        if (auto model = qobject_cast<ItemLibraryItemsModel *>(value.value<QObject *>()))
            return QVariant::fromValue(model);

        return value;
    }

    qWarning() << Q_FUNC_INFO << "invalid role requested";

    return {};
}

bool ItemLibraryCategoriesModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    // currently only categoryExpanded and categoryVisible properties is updatable
    if (index.isValid() && m_roleNames.contains(role)) {
        QVariant currValue = m_categoryList.at(index.row())->property(m_roleNames.value(role));

        if (currValue != value) {
            m_categoryList[index.row()]->setProperty(m_roleNames.value(role), value);
            if (m_roleNames.value(role) == "categoryExpanded") {
                ItemLibraryModel::saveExpandedState(value.toBool(),
                                                    m_categoryList[index.row()]->categoryName());
            } else if (m_roleNames.value(role) == "categoryVisible") {
                const ItemLibraryCategory *category = m_categoryList[index.row()];
                ItemLibraryModel::saveCategoryVisibleState(value.toBool(), category->categoryName(),
                                                           category->ownerImport()->importName());
            }
            emit dataChanged(index, index, {role});
            return true;
        }
    }
    return false;
}

QHash<int, QByteArray> ItemLibraryCategoriesModel::roleNames() const
{
    return m_roleNames;
}

void ItemLibraryCategoriesModel::expandCategories(bool expand)
{
    int i = 0;
    for (const auto &category : std::as_const(m_categoryList)) {
        if (category->categoryExpanded() != expand) {
            category->setExpanded(expand);
            ItemLibraryModel::saveExpandedState(expand, category->categoryName());
            emit dataChanged(index(i), index(i), {m_roleNames.key("categoryExpanded")});
        }
        ++i;
    }
}

void ItemLibraryCategoriesModel::addCategory(ItemLibraryCategory *category)
{
    m_categoryList.append(category);

    category->setVisible(true);
}

const QList<QPointer<ItemLibraryCategory>> &ItemLibraryCategoriesModel::categorySections() const
{
    return m_categoryList;
}

void ItemLibraryCategoriesModel::sortCategorySections()
{
    auto categorySort = [](ItemLibraryCategory *first, ItemLibraryCategory *second) {
        return QString::localeAwareCompare(first->sortingName(), second->sortingName()) < 0;
    };

    std::sort(m_categoryList.begin(), m_categoryList.end(), categorySort);

    for (const auto &category : std::as_const(m_categoryList))
        category->sortItems();
}

void ItemLibraryCategoriesModel::resetModel()
{
    beginResetModel();
    endResetModel();
}

bool ItemLibraryCategoriesModel::isAllCategoriesHidden() const
{
    for (const auto &category : std::as_const(m_categoryList)) {
        if (category->isCategoryVisible())
            return false;
    }

    return true;
}

void ItemLibraryCategoriesModel::showAllCategories()
{
    for (const auto &category : std::as_const(m_categoryList)) {
        if (!category->isCategoryVisible()) {
            category->setCategoryVisible(true);
            ItemLibraryModel::saveCategoryVisibleState(true, category->categoryName(),
                                                       category->ownerImport()->importName());
        }
    }

    emit dataChanged(index(0), index(m_categoryList.size() - 1), {m_roleNames.key("categoryVisible")});
}

void ItemLibraryCategoriesModel::hideCategory(const QString &categoryName)
{
    for (int i = 0; i < m_categoryList.size(); ++i) {
        const auto category = m_categoryList.at(i);
        if (category->categoryName() == categoryName) {
            category->setCategoryVisible(false);
            ItemLibraryModel::saveCategoryVisibleState(false, category->categoryName(),
                                                       category->ownerImport()->importName());
            emit dataChanged(index(i), index(i), {m_roleNames.key("categoryVisible")});
            break;
        }
    }
}

int ItemLibraryCategoriesModel::selectFirstVisibleCategory()
{
    for (int i = 0; i < m_categoryList.length(); ++i) {
        const auto category = m_categoryList.at(i);

        if (category->isCategoryVisible()) {
            category->setCategorySelected(true);
            emit dataChanged(index(i), index(i), {m_roleNames.key("categorySelected")});
            return i;
        }
    }

    return -1;
}

void ItemLibraryCategoriesModel::clearSelectedCategory(int categoryIndex)
{
    if (categoryIndex == -1 || m_categoryList.isEmpty())
        return;

    m_categoryList.at(categoryIndex)->setCategorySelected(false);
    emit dataChanged(index(categoryIndex), index(categoryIndex), {m_roleNames.key("categorySelected")});
}

QPointer<ItemLibraryCategory> ItemLibraryCategoriesModel::selectCategory(int categoryIndex)
{
    if (m_categoryList.isEmpty() || categoryIndex < 0 || categoryIndex >= m_categoryList.size())
        return nullptr;

    const QPointer<ItemLibraryCategory> category = m_categoryList.at(categoryIndex);

    if (!category->categorySelected()) {
        category->setCategorySelected(true);
        emit dataChanged(index(categoryIndex),index(categoryIndex), {m_roleNames.key("categorySelected")});
    }

    return category;
}

void ItemLibraryCategoriesModel::addRoleNames()
{
    int role = 0;
    const QMetaObject meta = ItemLibraryCategory::staticMetaObject;
    for (int i = meta.propertyOffset(); i < meta.propertyCount(); ++i)
        m_roleNames.insert(role++, meta.property(i).name());
}

} // namespace QmlDesigner
