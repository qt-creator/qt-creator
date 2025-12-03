// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "itemlibrarycategoriesmodel.h"
#include "itemlibrarycategory.h"
#include "itemlibrarymodel.h"
#include "itemlibrarytracing.h"

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QDebug>
#include <QVariant>
#include <QMetaProperty>
#include <QMetaObject>

namespace QmlDesigner {

using ItemLibraryTracing::category;

ItemLibraryCategoriesModel::ItemLibraryCategoriesModel(QObject *parent) :
    QAbstractListModel(parent)
{
    NanotraceHR::Tracer tracer{"item library categories model constructor", category()};

    addRoleNames();
}

ItemLibraryCategoriesModel::~ItemLibraryCategoriesModel()
{
    NanotraceHR::Tracer tracer{"item library categories model destructor", category()};
}

int ItemLibraryCategoriesModel::rowCount(const QModelIndex &) const
{
    NanotraceHR::Tracer tracer{"item library categories model row count", category()};

    return m_categoryList.size();
}

QVariant ItemLibraryCategoriesModel::data(const QModelIndex &index, int role) const
{
    NanotraceHR::Tracer tracer{"item library categories model data", category()};

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
    NanotraceHR::Tracer tracer{"item library categories model set data", category()};

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
    NanotraceHR::Tracer tracer{"item library categories model role names", category()};

    return m_roleNames;
}

void ItemLibraryCategoriesModel::expandCategories(bool expand)
{
    NanotraceHR::Tracer tracer{"item library categories model expand categories", category()};

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
    NanotraceHR::Tracer tracer{"item library categories model add category",
                               ItemLibraryTracing::category()};

    m_categoryList.append(category);

    category->setVisible(true);
}

const QList<QPointer<ItemLibraryCategory>> &ItemLibraryCategoriesModel::categorySections() const
{
    NanotraceHR::Tracer tracer{"item library categories model category sections", category()};

    return m_categoryList;
}

void ItemLibraryCategoriesModel::sortCategorySections()
{
    NanotraceHR::Tracer tracer{"item library categories model sort category sections", category()};

    auto categorySort = [](ItemLibraryCategory *first, ItemLibraryCategory *second) {
        return QString::localeAwareCompare(first->sortingName(), second->sortingName()) < 0;
    };

    std::sort(m_categoryList.begin(), m_categoryList.end(), categorySort);

    for (const auto &category : std::as_const(m_categoryList))
        category->sortItems();
}

void ItemLibraryCategoriesModel::resetModel()
{
    NanotraceHR::Tracer tracer{"item library categories model reset model", category()};

    beginResetModel();
    endResetModel();
}

bool ItemLibraryCategoriesModel::isAllCategoriesHidden() const
{
    NanotraceHR::Tracer tracer{"item library categories model is all categories hidden", category()};

    for (const auto &category : std::as_const(m_categoryList)) {
        if (category->isCategoryVisible())
            return false;
    }

    return true;
}

void ItemLibraryCategoriesModel::showAllCategories()
{
    NanotraceHR::Tracer tracer{"item library categories model show all categories", category()};

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
    NanotraceHR::Tracer tracer{"item library categories model hide category", category()};

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
    NanotraceHR::Tracer tracer{"item library categories model select first visible category",
                               category()};

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
    NanotraceHR::Tracer tracer{"item library categories model clear selected category", category()};

    if (categoryIndex == -1 || m_categoryList.isEmpty())
        return;

    m_categoryList.at(categoryIndex)->setCategorySelected(false);
    emit dataChanged(index(categoryIndex), index(categoryIndex), {m_roleNames.key("categorySelected")});
}

QPointer<ItemLibraryCategory> ItemLibraryCategoriesModel::selectCategory(int categoryIndex)
{
    NanotraceHR::Tracer tracer{"item library categories model select category", category()};

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
    NanotraceHR::Tracer tracer{"item library categories model add role names", category()};

    int role = 0;
    const QMetaObject meta = ItemLibraryCategory::staticMetaObject;
    for (int i = meta.propertyOffset(); i < meta.propertyCount(); ++i)
        m_roleNames.insert(role++, meta.property(i).name());
}

} // namespace QmlDesigner
