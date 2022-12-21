// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "categorysortfiltermodel.h"

#include <QRegularExpression>

namespace Utils {

CategorySortFilterModel::CategorySortFilterModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
}

void CategorySortFilterModel::setNewItemRole(int role)
{
    m_newItemRole = role;
    invalidate();
}

bool CategorySortFilterModel::filterAcceptsRow(int source_row,
                                               const QModelIndex &source_parent) const
{
    if (!source_parent.isValid()) {
        // category items should be visible if any of its children match
        const QRegularExpression &regexp = filterRegularExpression();
        const QModelIndex &categoryIndex = sourceModel()->index(source_row, 0, source_parent);
        if (regexp.match(sourceModel()->data(categoryIndex, filterRole()).toString()).hasMatch())
            return true;

        if (m_newItemRole != -1 && categoryIndex.isValid()) {
            if (categoryIndex.data(m_newItemRole).toBool())
                return true;
        }

        const int rowCount = sourceModel()->rowCount(categoryIndex);
        for (int row = 0; row < rowCount; ++row) {
            if (filterAcceptsRow(row, categoryIndex))
                return true;
        }
        return false;
    }

    if (m_newItemRole != -1) {
        const QModelIndex &idx = sourceModel()->index(source_row, 0, source_parent);
        if (idx.data(m_newItemRole).toBool())
            return true;
    }


    return QSortFilterProxyModel::filterAcceptsRow(source_row, source_parent);
}

} // Utils

