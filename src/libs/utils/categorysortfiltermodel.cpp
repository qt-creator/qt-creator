/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "categorysortfiltermodel.h"

namespace Utils {

CategorySortFilterModel::CategorySortFilterModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
}

bool CategorySortFilterModel::filterAcceptsRow(int source_row,
                                               const QModelIndex &source_parent) const
{
    if (!source_parent.isValid()) {
        // category items should be visible if any of its children match
        const QRegExp &regexp = filterRegExp();
        const QModelIndex &categoryIndex = sourceModel()->index(source_row, 0, source_parent);
        if (regexp.indexIn(sourceModel()->data(categoryIndex, filterRole()).toString()) != -1)
            return true;
        const int rowCount = sourceModel()->rowCount(categoryIndex);
        for (int row = 0; row < rowCount; ++row) {
            if (filterAcceptsRow(row, categoryIndex))
                return true;
        }
        return false;
    }
    return QSortFilterProxyModel::filterAcceptsRow(source_row, source_parent);
}

} // Utils

