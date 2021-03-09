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
    return m_categoryList.count();
}

QVariant ItemLibraryCategoriesModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_categoryList.count()) {
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
    // currently only categoryExpanded property is updatable
    if (index.isValid() && m_roleNames.contains(role)) {
        QVariant currValue = m_categoryList.at(index.row())->property(m_roleNames.value(role));
        if (currValue != value) {
            m_categoryList[index.row()]->setProperty(m_roleNames.value(role), value);
            if (m_roleNames.value(role) == "categoryExpanded") {
                ItemLibraryModel::saveExpandedState(value.toBool(),
                                                    m_categoryList[index.row()]->categoryName());
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

    for (const auto &category : qAsConst(m_categoryList))
        category->sortItems();
}

void ItemLibraryCategoriesModel::resetModel()
{
    beginResetModel();
    endResetModel();
}

void ItemLibraryCategoriesModel::addRoleNames()
{
    int role = 0;
    const QMetaObject meta = ItemLibraryCategory::staticMetaObject;
    for (int i = meta.propertyOffset(); i < meta.propertyCount(); ++i)
        m_roleNames.insert(role++, meta.property(i).name());
}

} // namespace QmlDesigner
