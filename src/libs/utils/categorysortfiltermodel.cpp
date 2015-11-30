/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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
        const int columnCount = sourceModel()->columnCount(categoryIndex);
        for (int row = 0; row < rowCount; ++row) {
            for (int column = 0; column < columnCount; ++column) {
                if (regexp.indexIn(sourceModel()->data(
                                       sourceModel()->index(row, column, categoryIndex),
                                       filterRole()).toString()) != -1)
                    return true;
            }
        }
        return false;
    }
    return QSortFilterProxyModel::filterAcceptsRow(source_row, source_parent);
}

} // Utils

